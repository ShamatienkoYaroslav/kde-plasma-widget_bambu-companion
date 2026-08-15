#include "LanPrinterConnection.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "../../protocol/BambuCommandBuilder.h"
#include "../../protocol/BambuReportParser.h"
#include "../../security/CertificateProbe.h"
#include "../../security/CertificateTrustStore.h"
#include "../../security/SecretStore.h"
#include "MqttClient.h"

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
}

LanPrinterConnection::LanPrinterConnection(const PrinterProfile &profile, QObject *parent)
    : PrinterConnection(parent)
    , m_profile(profile)
    , m_mqttClient(new MqttClient(this))
    , m_certificateProbe(new CertificateProbe(this))
{
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
    m_status = BambuReportParser::merge(m_status, doc.object());
    Q_EMIT statusUpdated(m_status);
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
