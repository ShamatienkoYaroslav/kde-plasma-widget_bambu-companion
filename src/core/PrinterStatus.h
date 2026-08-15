#pragma once

#include <QString>
#include <optional>

class PrinterStatus
{
public:
    enum class State {
        Unknown,
        Idle,
        Printing,
        Paused,
        Finished,
        Error,
        Offline,
    };

    State state = State::Unknown;
    std::optional<int> progressPercent;
    std::optional<int> currentLayer;
    std::optional<int> totalLayers;
    std::optional<double> nozzleTemp;
    std::optional<double> nozzleTargetTemp;
    std::optional<double> bedTemp;
    std::optional<double> bedTargetTemp;
    std::optional<double> chamberTemp;
    std::optional<int> coolingFanPercent;
    std::optional<int> auxFanPercent;
    std::optional<int> chamberFanPercent;
    std::optional<QString> speedProfile;
    std::optional<int> wifiSignalDbm;
    std::optional<int> remainingMinutes;

    static QString stateToString(State state);
};
