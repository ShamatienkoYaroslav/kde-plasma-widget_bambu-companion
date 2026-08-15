#include "ConnectionFactory.h"

#include "../transport/lan/LanPrinterConnection.h"
#include "PrinterProfile.h"

PrinterConnection *ConnectionFactory::create(const PrinterProfile &profile, QObject *parent)
{
    return new LanPrinterConnection(profile, parent);
}
