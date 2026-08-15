#include "PrinterStatus.h"

QString PrinterStatus::stateToString(State state)
{
    switch (state) {
    case State::Idle:
        return QStringLiteral("Idle");
    case State::Printing:
        return QStringLiteral("Printing");
    case State::Paused:
        return QStringLiteral("Paused");
    case State::Finished:
        return QStringLiteral("Finished");
    case State::Error:
        return QStringLiteral("Error");
    case State::Offline:
        return QStringLiteral("Offline");
    case State::Unknown:
    default:
        return QStringLiteral("Unknown");
    }
}
