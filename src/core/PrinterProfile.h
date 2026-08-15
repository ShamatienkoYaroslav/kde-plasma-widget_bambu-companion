#pragma once

#include <QString>
#include <QUuid>

struct PrinterProfile {
    QUuid id;
    QString name;
    QString host;
    QString serial;
    quint16 mqttPort = 8883;

    static PrinterProfile createLan(const QString &name, const QString &host, const QString &serial, quint16 mqttPort = 8883);
};
