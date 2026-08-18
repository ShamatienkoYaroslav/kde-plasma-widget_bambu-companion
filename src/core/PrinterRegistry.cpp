#include "PrinterRegistry.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include "../security/SecretStore.h"
#include "../transport/cloud/CloudAuthClient.h"
#include "../transport/cloud/CloudDeviceDirectory.h"
#include "../transport/cloud/CloudPrinterConnection.h"
#include "ConnectionFactory.h"

namespace
{
QString groupNameFor(const QUuid &id)
{
    return QStringLiteral("Printer %1").arg(id.toString());
}

// Explicit filename so printer profiles land in the applet's own config file
// rather than whichever host process's default config (e.g. plasmashellrc)
// KSharedConfig::openConfig() would otherwise resolve to.
KSharedConfigPtr openAppConfig()
{
    return KSharedConfig::openConfig(QStringLiteral("bambucompanionrc"));
}
}

PrinterRegistry &PrinterRegistry::instance()
{
    static PrinterRegistry registry;
    return registry;
}

PrinterRegistry::PrinterRegistry(QObject *parent)
    : QObject(parent)
    , m_cloudDeviceDirectory(new CloudDeviceDirectory(this))
{
    connect(m_cloudDeviceDirectory, &CloudDeviceDirectory::devicesReady, this, &PrinterRegistry::syncCloudDevices);
    connect(&CloudAuthClient::instance(), &CloudAuthClient::loginSucceeded, this, [this]() {
        m_cloudDeviceDirectory->fetchDevices();
    });
    connect(&CloudAuthClient::instance(), &CloudAuthClient::loggedOut, this, &PrinterRegistry::clearAllPrinters);

    loadPersistedPrinters();

    if (CloudAuthClient::instance().isLoggedIn()) {
        m_cloudDeviceDirectory->fetchDevices();
    } else if (!m_profiles.isEmpty()) {
        // The printer list is account-scoped: starting up already logged
        // out with leftover profiles (e.g. from before this behavior
        // existed, or a crash between logout and a fresh sync) is the same
        // "no source of truth" situation as an explicit logout.
        clearAllPrinters();
    }
}

void PrinterRegistry::syncCloudDevices(const QList<CloudDeviceInfo> &devices)
{
    for (const CloudDeviceInfo &device : devices) {
        bool alreadyExists = false;
        for (auto it = m_profiles.constBegin(); it != m_profiles.constEnd(); ++it) {
            if (it.value().serial == device.devId) {
                alreadyExists = true;
                break;
            }
        }
        if (!alreadyExists) {
            addCloudPrinter(device.devId, device.name);
        }
    }
}

void PrinterRegistry::clearAllPrinters()
{
    const QList<QUuid> ids = m_profiles.keys();
    for (const QUuid &id : ids) {
        removePrinter(id);
    }
}

void PrinterRegistry::loadPersistedPrinters()
{
    const KSharedConfigPtr config = openAppConfig();
    const QStringList groups = config->groupList();
    for (const QString &groupName : groups) {
        if (!groupName.startsWith(QLatin1String("Printer "))) {
            continue;
        }

        const KConfigGroup group = config->group(groupName);
        PrinterProfile profile;
        profile.id = QUuid(group.readEntry("Id"));
        profile.name = group.readEntry("Name");
        profile.host = group.readEntry("Host");
        profile.serial = group.readEntry("Serial");
        profile.mqttPort = static_cast<quint16>(group.readEntry("MqttPort", 8883));
        profile.mode = static_cast<PrinterProfile::ConnectionMode>(group.readEntry("Mode", static_cast<int>(PrinterProfile::ConnectionMode::LanOnly)));

        if (profile.id.isNull() || profile.serial.isEmpty()) {
            continue;
        }
        // Cloud-only printers have no host; LAN and LAN-preferred ones do.
        if (profile.mode != PrinterProfile::ConnectionMode::CloudOnly && profile.host.isEmpty()) {
            continue;
        }

        m_profiles.insert(profile.id, profile);
        startConnection(profile);
    }
}

void PrinterRegistry::persistProfile(const PrinterProfile &profile)
{
    const KSharedConfigPtr config = openAppConfig();
    KConfigGroup group = config->group(groupNameFor(profile.id));
    group.writeEntry("Id", profile.id.toString());
    group.writeEntry("Name", profile.name);
    group.writeEntry("Host", profile.host);
    group.writeEntry("Serial", profile.serial);
    group.writeEntry("MqttPort", static_cast<int>(profile.mqttPort));
    group.writeEntry("Mode", static_cast<int>(profile.mode));
    config->sync();
}

QUuid PrinterRegistry::addLanPrinter(const QString &name, const QString &host, const QString &serial, const QString &accessCode, quint16 mqttPort, PrinterProfile::ConnectionMode mode)
{
    PrinterProfile profile = PrinterProfile::createLan(name, host, serial, mqttPort);
    profile.mode = mode;

    SecretStore::instance().storeLanAccessCode(profile.id, accessCode);
    persistProfile(profile);

    m_profiles.insert(profile.id, profile);
    Q_EMIT printerAdded(profile.id);

    startConnection(profile);

    return profile.id;
}

QUuid PrinterRegistry::addCloudPrinter(const QString &devId, const QString &name)
{
    const PrinterProfile profile = PrinterProfile::createCloud(name, devId);

    persistProfile(profile);

    m_profiles.insert(profile.id, profile);
    Q_EMIT printerAdded(profile.id);

    startConnection(profile);

    return profile.id;
}

void PrinterRegistry::removePrinter(const QUuid &id)
{
    if (auto *connection = m_connections.take(id)) {
        connection->stop();
        connection->deleteLater();
    }
    m_profiles.remove(id);
    m_statuses.remove(id);
    m_cloudFallbackDone.remove(id);

    const KSharedConfigPtr config = openAppConfig();
    config->deleteGroup(groupNameFor(id));
    config->sync();

    SecretStore::instance().removeLanAccessCode(id);

    Q_EMIT printerRemoved(id);
}

void PrinterRegistry::startConnection(const PrinterProfile &profile)
{
    wireAndStart(profile.id, profile.mode, ConnectionFactory::create(profile, this));
}

void PrinterRegistry::wireAndStart(const QUuid &id, PrinterProfile::ConnectionMode mode, PrinterConnection *connection)
{
    m_connections.insert(id, connection);

    connect(connection, &PrinterConnection::statusUpdated, this, [this, id](const PrinterStatus &status) {
        m_statuses.insert(id, status);
        Q_EMIT printerStatusChanged(id);
    });

    connect(connection, &PrinterConnection::connectionStateChanged, this, [this, id, mode](PrinterConnection::ConnectionState state) {
        if (mode == PrinterProfile::ConnectionMode::PreferLanThenCloud && state == PrinterConnection::ConnectionState::Error) {
            fallBackToCloud(id);
        }
        Q_EMIT printerConnectionStateChanged(id);
    });

    connect(connection, &PrinterConnection::certificateTrustNeeded, this, [this, id](const QString &fingerprint) {
        Q_EMIT certificateTrustNeeded(id, fingerprint);
    });

    connect(connection, &PrinterConnection::commandAcked, this, [this, id](const QString &, bool success, const QString &reason) {
        if (!success) {
            Q_EMIT commandFailed(id, reason);
        }
    });

    connection->start();
}

void PrinterRegistry::fallBackToCloud(const QUuid &id)
{
    // Only fall back once per printer per process — avoids repeated
    // Error emissions (e.g. from a connection that keeps retrying and
    // keeps failing) re-triggering this and churning through connections.
    if (m_cloudFallbackDone.contains(id)) {
        return;
    }
    m_cloudFallbackDone.insert(id);

    auto *current = m_connections.value(id);
    if (!current) {
        return;
    }

    const PrinterProfile profile = m_profiles.value(id);

    current->disconnect(this);
    current->stop();
    current->deleteLater();
    m_connections.remove(id);

    wireAndStart(id, profile.mode, new CloudPrinterConnection(profile, this));
}

QList<PrinterProfile> PrinterRegistry::printers() const
{
    return m_profiles.values();
}

PrinterProfile PrinterRegistry::profile(const QUuid &id) const
{
    return m_profiles.value(id);
}

PrinterStatus PrinterRegistry::status(const QUuid &id) const
{
    return m_statuses.value(id);
}

PrinterConnection::ConnectionState PrinterRegistry::connectionState(const QUuid &id) const
{
    if (auto *connection = m_connections.value(id)) {
        return connection->connectionState();
    }
    return PrinterConnection::ConnectionState::Disconnected;
}

void PrinterRegistry::confirmCertificateTrust(const QUuid &id, const QString &fingerprint, bool accept)
{
    if (auto *connection = m_connections.value(id)) {
        connection->confirmCertificateTrust(fingerprint, accept);
    }
}

void PrinterRegistry::sendCommand(const QUuid &id, const PrinterCommand &command)
{
    if (auto *connection = m_connections.value(id)) {
        connection->sendCommand(command);
    }
}
