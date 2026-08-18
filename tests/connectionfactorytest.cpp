#include <memory>

#include <QTest>

#include "core/ConnectionFactory.h"
#include "core/PrinterProfile.h"
#include "transport/cloud/CloudPrinterConnection.h"
#include "transport/lan/LanPrinterConnection.h"

class ConnectionFactoryTest : public QObject
{
    Q_OBJECT

private slots:
    void lanOnlyProducesALanConnection()
    {
        const PrinterProfile profile = PrinterProfile::createLan(QStringLiteral("P"), QStringLiteral("203.0.113.1"), QStringLiteral("SER"));
        std::unique_ptr<PrinterConnection> connection(ConnectionFactory::create(profile));

        QVERIFY(qobject_cast<LanPrinterConnection *>(connection.get()) != nullptr);
        QVERIFY(qobject_cast<CloudPrinterConnection *>(connection.get()) == nullptr);
    }

    void cloudOnlyProducesACloudConnection()
    {
        const PrinterProfile profile = PrinterProfile::createCloud(QStringLiteral("P"), QStringLiteral("DEV1"));
        std::unique_ptr<PrinterConnection> connection(ConnectionFactory::create(profile));

        QVERIFY(qobject_cast<CloudPrinterConnection *>(connection.get()) != nullptr);
    }

    void preferLanThenCloudStartsOnLan()
    {
        PrinterProfile profile = PrinterProfile::createLan(QStringLiteral("P"), QStringLiteral("203.0.113.1"), QStringLiteral("SER"));
        profile.mode = PrinterProfile::ConnectionMode::PreferLanThenCloud;
        std::unique_ptr<PrinterConnection> connection(ConnectionFactory::create(profile));

        QVERIFY(qobject_cast<LanPrinterConnection *>(connection.get()) != nullptr);
    }
};

QTEST_MAIN(ConnectionFactoryTest)
#include "connectionfactorytest.moc"
