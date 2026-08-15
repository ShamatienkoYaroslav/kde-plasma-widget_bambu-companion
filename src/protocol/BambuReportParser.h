#pragma once

#include <QJsonObject>

#include "../core/PrinterStatus.h"

class BambuReportParser
{
public:
    // Merges a raw MQTT report payload into `previous`, leaving any field not
    // present in `report` unchanged (Bambu printers send full reports on
    // connect and delta reports afterward).
    static PrinterStatus merge(const PrinterStatus &previous, const QJsonObject &report);
};
