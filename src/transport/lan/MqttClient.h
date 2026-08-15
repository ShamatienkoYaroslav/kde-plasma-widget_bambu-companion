#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

struct mosquitto;
struct mosquitto_message;

class MqttClient : public QObject
{
    Q_OBJECT

public:
    explicit MqttClient(QObject *parent = nullptr);
    ~MqttClient() override;

    void connectToHost(const QString &host, quint16 port, const QString &username, const QString &password);
    void disconnectFromHost();
    void subscribe(const QString &topic);
    void publish(const QString &topic, const QByteArray &payload);

    bool isConnected() const;

Q_SIGNALS:
    void connected();
    void disconnected(const QString &reason);
    void messageReceived(const QString &topic, const QByteArray &payload);
    void errorOccurred(const QString &message);

private:
    static void onConnect(mosquitto *mosq, void *userdata, int rc);
    static void onDisconnect(mosquitto *mosq, void *userdata, int rc);
    static void onMessage(mosquitto *mosq, void *userdata, const mosquitto_message *message);

    mosquitto *m_mosq = nullptr;
    bool m_connected = false;
};
