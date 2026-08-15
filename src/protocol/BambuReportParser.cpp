#include "BambuReportParser.h"

#include <QJsonValue>

namespace
{

PrinterStatus::State stateFromGcodeState(const QString &value)
{
    if (value == QLatin1String("IDLE")) {
        return PrinterStatus::State::Idle;
    }
    if (value == QLatin1String("RUNNING") || value == QLatin1String("PREPARE")) {
        return PrinterStatus::State::Printing;
    }
    if (value == QLatin1String("PAUSE")) {
        return PrinterStatus::State::Paused;
    }
    if (value == QLatin1String("FINISH")) {
        return PrinterStatus::State::Finished;
    }
    if (value == QLatin1String("FAILED")) {
        return PrinterStatus::State::Error;
    }
    return PrinterStatus::State::Unknown;
}

std::optional<double> readDouble(const QJsonObject &object, const QString &key)
{
    if (!object.contains(key)) {
        return std::nullopt;
    }
    const QJsonValue value = object.value(key);
    if (value.isDouble()) {
        return value.toDouble();
    }
    if (value.isString()) {
        bool ok = false;
        const double parsed = value.toString().toDouble(&ok);
        if (ok) {
            return parsed;
        }
    }
    return std::nullopt;
}

std::optional<int> readInt(const QJsonObject &object, const QString &key)
{
    if (auto value = readDouble(object, key)) {
        return static_cast<int>(*value);
    }
    return std::nullopt;
}

std::optional<QString> readString(const QJsonObject &object, const QString &key)
{
    if (!object.contains(key)) {
        return std::nullopt;
    }
    const QJsonValue value = object.value(key);
    if (value.isString()) {
        return value.toString();
    }
    return std::nullopt;
}

QString speedProfileFromLevel(int level)
{
    switch (level) {
    case 1:
        return QStringLiteral("Silent");
    case 2:
        return QStringLiteral("Standard");
    case 3:
        return QStringLiteral("Sport");
    case 4:
        return QStringLiteral("Ludicrous");
    default:
        return QStringLiteral("Unknown");
    }
}

} // namespace

PrinterStatus BambuReportParser::merge(const PrinterStatus &previous, const QJsonObject &report)
{
    PrinterStatus status = previous;

    const QJsonObject print = report.value(QStringLiteral("print")).toObject();
    if (print.isEmpty()) {
        return status;
    }

    if (auto gcodeState = readString(print, QStringLiteral("gcode_state"))) {
        status.state = stateFromGcodeState(*gcodeState);
    }
    if (auto value = readInt(print, QStringLiteral("mc_percent"))) {
        status.progressPercent = value;
    }
    if (auto value = readInt(print, QStringLiteral("mc_remaining_time"))) {
        status.remainingMinutes = value;
    }
    if (auto value = readInt(print, QStringLiteral("layer_num"))) {
        status.currentLayer = value;
    }
    if (auto value = readInt(print, QStringLiteral("total_layer_num"))) {
        status.totalLayers = value;
    }
    if (auto value = readDouble(print, QStringLiteral("nozzle_temper"))) {
        status.nozzleTemp = value;
    }
    if (auto value = readDouble(print, QStringLiteral("nozzle_target_temper"))) {
        status.nozzleTargetTemp = value;
    }
    if (auto value = readDouble(print, QStringLiteral("bed_temper"))) {
        status.bedTemp = value;
    }
    if (auto value = readDouble(print, QStringLiteral("bed_target_temper"))) {
        status.bedTargetTemp = value;
    }
    if (auto value = readDouble(print, QStringLiteral("chamber_temper"))) {
        status.chamberTemp = value;
    }
    if (auto value = readInt(print, QStringLiteral("cooling_fan_speed"))) {
        status.coolingFanPercent = value;
    }
    if (auto value = readInt(print, QStringLiteral("big_fan1_speed"))) {
        status.auxFanPercent = value;
    }
    if (auto value = readInt(print, QStringLiteral("big_fan2_speed"))) {
        status.chamberFanPercent = value;
    }
    if (auto value = readInt(print, QStringLiteral("spd_lvl"))) {
        status.speedProfile = speedProfileFromLevel(*value);
    }
    if (auto value = readString(print, QStringLiteral("wifi_signal"))) {
        QString signal = *value;
        signal.remove(QStringLiteral("dBm"));
        bool ok = false;
        const int dbm = signal.toInt(&ok);
        if (ok) {
            status.wifiSignalDbm = dbm;
        }
    }

    return status;
}
