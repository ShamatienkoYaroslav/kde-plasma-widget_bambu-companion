#include "CloudPrinterConnection.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "../../protocol/BambuCommandBuilder.h"
#include "../../protocol/BambuReportParser.h"
#include "../lan/MqttClient.h"
#include "../lan/PendingCommandTracker.h"
#include "CloudAuthClient.h"

namespace
{
// Hardcoded to the US region for now — see design.md's Risks section on
// region selection/detection for non-US accounts.
constexpr auto kCloudMqttHost = "us.mqtt.bambulab.com";
constexpr quint16 kCloudMqttPort = 8883;
constexpr int kCommandTimeoutMs = 10000;

QString reportTopic(const QString &serial)
{
    return QStringLiteral("device/%1/report").arg(serial);
}

QString requestTopic(const QString &serial)
{
    return QStringLiteral("device/%1/request").arg(serial);
}
}

CloudPrinterConnection::CloudPrinterConnection(const PrinterProfile &profile, QObject *parent)
    : PrinterConnection(parent)
    , m_profile(profile)
    , m_mqttClient(new MqttClient(this))
    , m_commandTracker(new PendingCommandTracker(this))
{
    connect(m_commandTracker, &PendingCommandTracker::acked, this, &PrinterConnection::commandAcked);

    connect(m_mqttClient, &MqttClient::connected, this, [this]() {
        setState(ConnectionState::Connected);
        m_mqttClient->subscribe(reportTopic(m_profile.serial));
        m_mqttClient->publish(requestTopic(m_profile.serial), BambuCommandBuilder::pushAll());
    });

    connect(m_mqttClient, &MqttClient::disconnected, this, [this](const QString &) {
        setState(ConnectionState::Disconnected);
        m_status.state = PrinterStatus::State::Offline;
        Q_EMIT statusUpdated(m_status);
    });

    connect(m_mqttClient, &MqttClient::messageReceived, this, [this](const QString &topic, const QByteArray &payload) {
        handleMessage(topic, payload);
    });

    connect(m_mqttClient, &MqttClient::errorOccurred, this, [this](const QString &) {
        setState(ConnectionState::Error);
    });

    // The account's user id (needed for the MQTT username) requires its own
    // HTTP round trip — see CloudAuthClient::ensureUserId(). Every
    // CloudPrinterConnection shares that one singleton's in-flight
    // request/result, so this just reacts once it's available.
    connect(&CloudAuthClient::instance(), &CloudAuthClient::userIdReady, this, [this](const QString &) {
        if (m_state == ConnectionState::Connecting) {
            openMqttSession();
        }
    });
    connect(&CloudAuthClient::instance(), &CloudAuthClient::userIdFetchFailed, this, [this](const QString &) {
        if (m_state == ConnectionState::Connecting) {
            setState(ConnectionState::Error);
        }
    });
}

void CloudPrinterConnection::start()
{
    if (!CloudAuthClient::instance().isLoggedIn()) {
        setState(ConnectionState::Error);
        return;
    }

    setState(ConnectionState::Connecting);
    CloudAuthClient::instance().ensureUserId();
}

void CloudPrinterConnection::openMqttSession()
{
    const QString username = QStringLiteral("u_%1").arg(CloudAuthClient::instance().cachedUserId());
    const QString password = CloudAuthClient::instance().accessToken();
    m_mqttClient->connectToHost(QString::fromLatin1(kCloudMqttHost), kCloudMqttPort, username, password, /*useSystemCaTrust=*/true);
}

void CloudPrinterConnection::stop()
{
    m_mqttClient->disconnectFromHost();
    setState(ConnectionState::Disconnected);
}

QString CloudPrinterConnection::sendCommand(const PrinterCommand &command)
{
    const QString sequenceId = QString::number(m_nextSequenceId++);
    const QByteArray payload = BambuCommandBuilder::build(sequenceId, command);

    m_commandTracker->track(sequenceId, kCommandTimeoutMs);
    m_mqttClient->publish(requestTopic(m_profile.serial), payload);

    return sequenceId;
}

void CloudPrinterConnection::handleMessage(const QString &topic, const QByteArray &payload)
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
        const QString result = print.value(QStringLiteral("result")).toString();
        const bool success = result.isEmpty() || result.compare(QStringLiteral("success"), Qt::CaseInsensitive) == 0;
        const QString reason = print.value(QStringLiteral("reason")).toString();
        m_commandTracker->resolve(sequenceId, success, reason);
    }

    m_status = BambuReportParser::merge(m_status, root);
    Q_EMIT statusUpdated(m_status);
}

PrinterConnection::ConnectionState CloudPrinterConnection::connectionState() const
{
    return m_state;
}

PrinterStatus CloudPrinterConnection::status() const
{
    return m_status;
}

void CloudPrinterConnection::setState(ConnectionState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    Q_EMIT connectionStateChanged(m_state);
}
