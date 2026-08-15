#include "PrinterProfile.h"

PrinterProfile PrinterProfile::createLan(const QString &name, const QString &host, const QString &serial, quint16 mqttPort)
{
    PrinterProfile profile;
    profile.id = QUuid::createUuid();
    profile.name = name;
    profile.host = host;
    profile.serial = serial;
    profile.mqttPort = mqttPort;
    return profile;
}
