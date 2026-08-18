#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include "../core/PrinterCommand.h"

class BambuCommandBuilder
{
public:
    // Requests a full status report; publish once per connection (Bambu
    // printers stream delta reports afterward, so polling this is unnecessary
    // and discouraged by the community-documented protocol).
    static QByteArray pushAll();

    static QByteArray pause(const QString &sequenceId);
    static QByteArray resume(const QString &sequenceId);
    static QByteArray stop(const QString &sequenceId);
    static QByteArray skipObjects(const QString &sequenceId, const QList<int> &objectIds);

    // Dispatches to the builder above matching command.type — shared by
    // every PrinterConnection implementation's sendCommand().
    static QByteArray build(const QString &sequenceId, const PrinterCommand &command);
};
