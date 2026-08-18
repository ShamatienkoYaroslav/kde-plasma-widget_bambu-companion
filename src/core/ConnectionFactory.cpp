#include "ConnectionFactory.h"

#include "../transport/cloud/CloudPrinterConnection.h"
#include "../transport/lan/LanPrinterConnection.h"
#include "PrinterProfile.h"

PrinterConnection *ConnectionFactory::create(const PrinterProfile &profile, QObject *parent)
{
    switch (profile.mode) {
    case PrinterProfile::ConnectionMode::CloudOnly:
        return new CloudPrinterConnection(profile, parent);
    case PrinterProfile::ConnectionMode::LanOnly:
    case PrinterProfile::ConnectionMode::PreferLanThenCloud:
    default:
        // PreferLanThenCloud starts on LAN; PrinterRegistry swaps in a
        // CloudPrinterConnection as a fallback if this one settles into
        // ConnectionState::Error (see PrinterRegistry::startConnection()).
        return new LanPrinterConnection(profile, parent);
    }
}
