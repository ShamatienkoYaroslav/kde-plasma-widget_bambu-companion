#pragma once

#include "../../core/PrinterConnection.h"
#include "../../core/PrinterProfile.h"

class MqttClient;
class PendingCommandTracker;

class CloudPrinterConnection : public PrinterConnection
{
    Q_OBJECT

public:
    explicit CloudPrinterConnection(const PrinterProfile &profile, QObject *parent = nullptr);

    void start() override;
    void stop() override;
    ConnectionState connectionState() const override;
    PrinterStatus status() const override;
    QString sendCommand(const PrinterCommand &command) override;
    // No confirmCertificateTrust() override — the relay uses a
    // publicly-trusted certificate, so the inherited no-op default applies.

private:
    void handleMessage(const QString &topic, const QByteArray &payload);
    void setState(ConnectionState state);
    void openMqttSession();

    PrinterProfile m_profile;
    MqttClient *m_mqttClient = nullptr;
    PendingCommandTracker *m_commandTracker = nullptr;
    PrinterStatus m_status;
    ConnectionState m_state = ConnectionState::Disconnected;
    quint64 m_nextSequenceId = 1;
};
