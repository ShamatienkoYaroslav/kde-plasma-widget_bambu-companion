#include "PrinterController.h"

#include "../core/PrinterProfile.h"
#include "../core/PrinterRegistry.h"

PrinterController::PrinterController(QObject *parent)
    : QObject(parent)
{
    connect(&PrinterRegistry::instance(), &PrinterRegistry::certificateTrustNeeded, this, [this](const QUuid &id, const QString &fingerprint) {
        const PrinterProfile p = PrinterRegistry::instance().profile(id);
        Q_EMIT certificateTrustNeeded(id.toString(), p.name, fingerprint);
    });
}

void PrinterController::addLanPrinter(const QString &name, const QString &host, const QString &serial, const QString &accessCode, int mqttPort)
{
    PrinterRegistry::instance().addLanPrinter(name, host, serial, accessCode, static_cast<quint16>(mqttPort));
}

void PrinterController::removePrinter(const QString &printerId)
{
    PrinterRegistry::instance().removePrinter(QUuid(printerId));
}

void PrinterController::confirmCertificateTrust(const QString &printerId, const QString &fingerprint, bool accept)
{
    PrinterRegistry::instance().confirmCertificateTrust(QUuid(printerId), fingerprint, accept);
}
