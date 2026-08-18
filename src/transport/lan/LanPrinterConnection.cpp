#include "LanPrinterConnection.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "../../protocol/BambuCommandBuilder.h"
#include "../../protocol/BambuReportParser.h"
#include "../../security/CertificateProbe.h"
#include "../../security/CertificateTrustStore.h"
#include "../../security/SecretStore.h"
#include "MqttClient.h"
#include "PendingCommandTracker.h"

namespace
{
QString reportTopic(const QString &serial)
{
    return QStringLiteral("device/%1/report").arg(serial);
}

QString requestTopic(const QString &serial)
{
    return QStringLiteral("device/%1/request").arg(serial);
}

constexpr int kCommandTimeoutMs = 10000;
}

LanPrinterConnection::LanPrinterConnection(const PrinterProfile &profile, QObject *parent)
    : PrinterConnection(parent)
    , m_profile(profile)
    , m_mqttClient(new MqttClient(this))
    , m_certificateProbe(new CertificateProbe(this))
    , m_commandTracker(new PendingCommandTracker(this))
{
    connect(m_commandTracker, &PendingCommandTracker::acked, this, &PrinterConnection::commandAcked);

    connect(m_mqttClient, &MqttClient::connected, this, [this]() {
        setState(ConnectionState::Connected);
        m_mqttClient->subscribe(reportTopic(m_profile.serial));
        m_mqttClient->publish(requestTopic(m_profile.serial), BambuCommandBuilder::pushAll());
    });

    // libmosquitto's own background thread (started via mosquitto_loop_start)
    // automatically retries an unexpected disconnect, so no manual reconnect
    // logic is needed here beyond reflecting the state change and letting the
    // `connected` handler above re-subscribe once it succeeds.
    connect(m_mqttClient, &MqttClient::disconnected, this, [this](const QString &) {
        setState(ConnectionState::Disconnected);
        m_status.state = PrinterStatus::State::Offline;
        Q_EMIT statusUpdated(m_status);
    });

    connect(m_mqttClient, &MqttClient::messageReceived, this, [this](const QString &topic, const QByteArray &payload) {
        handleMessage(topic, payload);
    });

    connect(m_certificateProbe, &CertificateProbe::fingerprintReady, this, [this](const QString &fingerprint) {
        auto &trustStore = CertificateTrustStore::instance();
        if (trustStore.isPinned(m_profile.serial)) {
            if (trustStore.isTrusted(m_profile.serial, fingerprint)) {
                openMqttSession();
            } else {
                m_pendingFingerprint = fingerprint;
                setState(ConnectionState::Error);
                Q_EMIT certificateTrustNeeded(fingerprint);
            }
        } else {
            m_pendingFingerprint = fingerprint;
            setState(ConnectionState::AwaitingCertificateTrust);
            Q_EMIT certificateTrustNeeded(fingerprint);
        }
    });

    connect(m_certificateProbe, &CertificateProbe::probeFailed, this, [this](const QString &) {
        setState(ConnectionState::Error);
    });
}

void LanPrinterConnection::start()
{
    setState(ConnectionState::Connecting);
    beginCertificateProbe();
}

void LanPrinterConnection::stop()
{
    m_mqttClient->disconnectFromHost();
    setState(ConnectionState::Disconnected);
}

void LanPrinterConnection::beginCertificateProbe()
{
    m_certificateProbe->probe(m_profile.host, m_profile.mqttPort);
}

void LanPrinterConnection::confirmCertificateTrust(const QString &fingerprint, bool accept)
{
    if (fingerprint != m_pendingFingerprint) {
        return;
    }

    if (accept) {
        CertificateTrustStore::instance().trust(m_profile.serial, fingerprint);
        openMqttSession();
    } else {
        setState(ConnectionState::Error);
    }
}

void LanPrinterConnection::openMqttSession()
{
    setState(ConnectionState::Connecting);
    const QString accessCode = SecretStore::instance().lanAccessCode(m_profile.id);
    m_mqttClient->connectToHost(m_profile.host, m_profile.mqttPort, QStringLiteral("bblp"), accessCode);
}

void LanPrinterConnection::handleMessage(const QString &topic, const QByteArray &payload)
{
    Q_UNUSED(topic);
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonObject print = root.value(QStringLiteral("print")).toObject();
    const QString sequenceId = print.value(QStringLiteral("sequence_id")).toString();
    if (!sequenceId.isEmpty()) {
        // The community-documented protocol indicates the printer echoes the
        // command's sequence_id back with a "result" field; a matching
        // sequence_id with no explicit failure marker is treated as success
        // (see design.md's Risks section — this is a best-effort assumption,
        // pending confirmation against real hardware). resolve() is a no-op
        // if this sequence_id isn't actually pending (e.g. it's "0" from our
        // own pushall request).
        const QString result = print.value(QStringLiteral("result")).toString();
        const bool success = result.isEmpty() || result.compare(QStringLiteral("success"), Qt::CaseInsensitive) == 0;
        const QString reason = print.value(QStringLiteral("reason")).toString();
        m_commandTracker->resolve(sequenceId, success, reason);
    }

    m_status = BambuReportParser::merge(m_status, root);
    Q_EMIT statusUpdated(m_status);
}

QString LanPrinterConnection::sendCommand(const PrinterCommand &command)
{
    const QString sequenceId = QString::number(m_nextSequenceId++);
    const QByteArray payload = BambuCommandBuilder::build(sequenceId, command);

    m_commandTracker->track(sequenceId, kCommandTimeoutMs);
    m_mqttClient->publish(requestTopic(m_profile.serial), payload);

    return sequenceId;
}

PrinterConnection::ConnectionState LanPrinterConnection::connectionState() const
{
    return m_state;
}

PrinterStatus LanPrinterConnection::status() const
{
    return m_status;
}

void LanPrinterConnection::setState(ConnectionState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    Q_EMIT connectionStateChanged(m_state);
}
