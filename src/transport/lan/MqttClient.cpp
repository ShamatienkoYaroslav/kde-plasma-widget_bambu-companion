#include "MqttClient.h"

#include <mosquitto.h>

namespace
{
bool mosquittoLibInitialized = false;

void ensureLibraryInitialized()
{
    if (!mosquittoLibInitialized) {
        mosquitto_lib_init();
        mosquittoLibInitialized = true;
    }
}
}

MqttClient::MqttClient(QObject *parent)
    : QObject(parent)
{
    ensureLibraryInitialized();

    m_mosq = mosquitto_new(nullptr, true, this);
    mosquitto_connect_callback_set(m_mosq, &MqttClient::onConnect);
    mosquitto_disconnect_callback_set(m_mosq, &MqttClient::onDisconnect);
    mosquitto_message_callback_set(m_mosq, &MqttClient::onMessage);
}

MqttClient::~MqttClient()
{
    if (m_mosq) {
        mosquitto_loop_stop(m_mosq, true);
        mosquitto_destroy(m_mosq);
    }
}

void MqttClient::connectToHost(const QString &host, quint16 port, const QString &username, const QString &password)
{
    mosquitto_username_pw_set(m_mosq, username.toUtf8().constData(), password.toUtf8().constData());

    // Printers use self-signed certificates; skip libmosquitto's own chain
    // validation here since the exact certificate has already been vetted
    // via CertificateProbe/CertificateTrustStore before this call is made.
    mosquitto_tls_set(m_mosq, nullptr, nullptr, nullptr, nullptr, nullptr);
    mosquitto_tls_insecure_set(m_mosq, true);

    const int rc = mosquitto_connect_async(m_mosq, host.toUtf8().constData(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        Q_EMIT errorOccurred(QString::fromUtf8(mosquitto_strerror(rc)));
        return;
    }

    mosquitto_loop_start(m_mosq);
}

void MqttClient::disconnectFromHost()
{
    if (m_mosq) {
        mosquitto_disconnect(m_mosq);
    }
}

void MqttClient::subscribe(const QString &topic)
{
    if (m_mosq) {
        mosquitto_subscribe(m_mosq, nullptr, topic.toUtf8().constData(), 0);
    }
}

void MqttClient::publish(const QString &topic, const QByteArray &payload)
{
    if (m_mosq) {
        mosquitto_publish(m_mosq, nullptr, topic.toUtf8().constData(), payload.size(), payload.constData(), 0, false);
    }
}

bool MqttClient::isConnected() const
{
    return m_connected;
}

void MqttClient::onConnect(mosquitto *, void *userdata, int rc)
{
    auto *self = static_cast<MqttClient *>(userdata);
    if (rc == 0) {
        self->m_connected = true;
        Q_EMIT self->connected();
    } else {
        Q_EMIT self->errorOccurred(QString::fromUtf8(mosquitto_connack_string(rc)));
    }
}

void MqttClient::onDisconnect(mosquitto *, void *userdata, int rc)
{
    auto *self = static_cast<MqttClient *>(userdata);
    self->m_connected = false;
    Q_EMIT self->disconnected(QString::fromUtf8(mosquitto_strerror(rc)));
}

void MqttClient::onMessage(mosquitto *, void *userdata, const mosquitto_message *message)
{
    auto *self = static_cast<MqttClient *>(userdata);
    if (!message || !message->topic) {
        return;
    }
    const QString topic = QString::fromUtf8(message->topic);
    const QByteArray payload(static_cast<const char *>(message->payload), message->payloadlen);
    Q_EMIT self->messageReceived(topic, payload);
}
