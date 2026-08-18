#include "PrinterController.h"

#include "../core/PrinterCommand.h"
#include "../core/PrinterProfile.h"
#include "../core/PrinterRegistry.h"

PrinterController::PrinterController(QObject *parent)
    : QObject(parent)
{
    connect(&PrinterRegistry::instance(), &PrinterRegistry::certificateTrustNeeded, this, [this](const QUuid &id, const QString &fingerprint) {
        const PrinterProfile p = PrinterRegistry::instance().profile(id);
        Q_EMIT certificateTrustNeeded(id.toString(), p.name, fingerprint);
    });

    connect(&PrinterRegistry::instance(), &PrinterRegistry::commandFailed, this, [this](const QUuid &id, const QString &reason) {
        Q_EMIT commandFailed(id.toString(), reason);
    });
}

void PrinterController::removePrinter(const QString &printerId)
{
    PrinterRegistry::instance().removePrinter(QUuid(printerId));
}

void PrinterController::confirmCertificateTrust(const QString &printerId, const QString &fingerprint, bool accept)
{
    PrinterRegistry::instance().confirmCertificateTrust(QUuid(printerId), fingerprint, accept);
}

void PrinterController::pause(const QString &printerId)
{
    PrinterRegistry::instance().sendCommand(QUuid(printerId), PrinterCommand{PrinterCommand::Type::Pause, {}});
}

void PrinterController::resume(const QString &printerId)
{
    PrinterRegistry::instance().sendCommand(QUuid(printerId), PrinterCommand{PrinterCommand::Type::Resume, {}});
}

void PrinterController::stop(const QString &printerId)
{
    PrinterRegistry::instance().sendCommand(QUuid(printerId), PrinterCommand{PrinterCommand::Type::Stop, {}});
}

void PrinterController::skipObjects(const QString &printerId, const QList<int> &objectIds)
{
    PrinterRegistry::instance().sendCommand(QUuid(printerId), PrinterCommand{PrinterCommand::Type::SkipObjects, objectIds});
}
