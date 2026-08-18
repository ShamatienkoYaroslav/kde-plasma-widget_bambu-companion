#pragma once

#include <QString>
#include <QUuid>

struct PrinterProfile {
    enum class ConnectionMode {
        LanOnly,
        CloudOnly,
        PreferLanThenCloud,
    };

    QUuid id;
    QString name;
    QString host;
    QString serial;
    quint16 mqttPort = 8883;
    ConnectionMode mode = ConnectionMode::LanOnly;

    static PrinterProfile createLan(const QString &name, const QString &host, const QString &serial, quint16 mqttPort = 8883);
    static PrinterProfile createCloud(const QString &name, const QString &devId);
};
