#include <algorithm>

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "core/PrinterRegistry.h"

// Exercises PrinterRegistry::addLanPrinter end-to-end (persistence + secret
// storage). This deliberately touches real KConfig (isolated below via
// XDG_CONFIG_HOME) and real KWallet (not isolated — requires a working
// kwalletd, consistent with this project's testing conventions: transport-
// and system-service-adjacent code is otherwise verified manually). The
// connection attempt addLanPrinter triggers is asynchronous and never
// progresses during this synchronous test, so it does not affect the result.
class PrinterRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_configDir.isValid());
        qputenv("XDG_CONFIG_HOME", m_configDir.path().toUtf8());
    }

    void addingAPrinterPersistsNonSecretFieldsWithoutTheAccessCode()
    {
        auto &registry = PrinterRegistry::instance();
        const QUuid id = registry.addLanPrinter(QStringLiteral("My Printer"),
                                                 QStringLiteral("203.0.113.5"),
                                                 QStringLiteral("SERIALXYZ"),
                                                 QStringLiteral("s3cr3t-access-code"),
                                                 8883);

        QVERIFY(!id.isNull());

        const PrinterProfile profile = registry.profile(id);
        QCOMPARE(profile.name, QStringLiteral("My Printer"));
        QCOMPARE(profile.host, QStringLiteral("203.0.113.5"));
        QCOMPARE(profile.serial, QStringLiteral("SERIALXYZ"));
        QCOMPARE(profile.mqttPort, static_cast<quint16>(8883));

        QFile configFile(m_configDir.path() + QStringLiteral("/bambucompanionrc"));
        QVERIFY(configFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString contents = QString::fromUtf8(configFile.readAll());

        QVERIFY(contents.contains(QStringLiteral("203.0.113.5")));
        QVERIFY(contents.contains(QStringLiteral("SERIALXYZ")));
        QVERIFY(!contents.contains(QStringLiteral("s3cr3t-access-code")));
    }

    void addingACloudPrinterPersistsCloudOnlyMode()
    {
        auto &registry = PrinterRegistry::instance();
        const QUuid id = registry.addCloudPrinter(QStringLiteral("DEV42"), QStringLiteral("Cloud Printer"));

        QVERIFY(!id.isNull());

        const PrinterProfile profile = registry.profile(id);
        QCOMPARE(profile.name, QStringLiteral("Cloud Printer"));
        QCOMPARE(profile.serial, QStringLiteral("DEV42"));
        QCOMPARE(profile.mode, PrinterProfile::ConnectionMode::CloudOnly);
    }

    void preferLanThenCloudFallsBackAfterLanFails()
    {
        auto &registry = PrinterRegistry::instance();

        // Port 1 on loopback: nothing listens there, so the certificate
        // probe's TCP connect is refused almost immediately rather than
        // timing out, keeping this test fast.
        const QUuid id = registry.addLanPrinter(QStringLiteral("Fallback Printer"),
                                                 QStringLiteral("127.0.0.1"),
                                                 QStringLiteral("FALLBACKSERIAL"),
                                                 QStringLiteral("code"),
                                                 1,
                                                 PrinterProfile::ConnectionMode::PreferLanThenCloud);
        QVERIFY(!id.isNull());

        QSignalSpy stateChangedSpy(&registry, &PrinterRegistry::printerConnectionStateChanged);

        // The LAN attempt fails (Error), PrinterRegistry falls back to a
        // CloudPrinterConnection, which itself immediately fails too (not
        // logged into a Bambu account in this test) — so at least two
        // state-change emissions for this printer indicates the fallback
        // path ran, not just the initial LAN failure alone.
        QTRY_VERIFY_WITH_TIMEOUT(std::count_if(stateChangedSpy.begin(),
                                                stateChangedSpy.end(),
                                                [&id](const QList<QVariant> &args) {
                                                    return args.at(0).value<QUuid>() == id;
                                                })
                                      >= 2,
                                  5000);
    }

private:
    QTemporaryDir m_configDir;
};

QTEST_MAIN(PrinterRegistryTest)
#include "printerregistrytest.moc"
