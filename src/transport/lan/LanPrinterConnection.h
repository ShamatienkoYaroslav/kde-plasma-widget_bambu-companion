#pragma once

#include "../../core/PrinterConnection.h"
#include "../../core/PrinterProfile.h"

class MqttClient;
class CertificateProbe;
class PendingCommandTracker;

class LanPrinterConnection : public PrinterConnection
{
    Q_OBJECT

public:
    explicit LanPrinterConnection(const PrinterProfile &profile, QObject *parent = nullptr);

    void start() override;
    void stop() override;
    ConnectionState connectionState() const override;
    PrinterStatus status() const override;
    void confirmCertificateTrust(const QString &fingerprint, bool accept) override;
    QString sendCommand(const PrinterCommand &command) override;

private:
    void beginCertificateProbe();
    void openMqttSession();
    void handleMessage(const QString &topic, const QByteArray &payload);
    void setState(ConnectionState state);

    PrinterProfile m_profile;
    MqttClient *m_mqttClient = nullptr;
    CertificateProbe *m_certificateProbe = nullptr;
    PendingCommandTracker *m_commandTracker = nullptr;
    PrinterStatus m_status;
    ConnectionState m_state = ConnectionState::Disconnected;
    QString m_pendingFingerprint;
    quint64 m_nextSequenceId = 1;
};
