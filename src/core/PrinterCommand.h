#pragma once

#include <QList>

struct PrinterCommand {
    enum class Type {
        Pause,
        Resume,
        Stop,
        SkipObjects,
    };

    Type type = Type::Pause;
    QList<int> objectIds; // only meaningful for Type::SkipObjects
};
