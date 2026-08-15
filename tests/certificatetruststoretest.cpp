#include <QTemporaryDir>
#include <QTest>

#include "security/CertificateTrustStore.h"

class CertificateTrustStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_configDir.isValid());
        qputenv("XDG_CONFIG_HOME", m_configDir.path().toUtf8());
    }

    void unpinnedSerialIsUntrusted()
    {
        QVERIFY(!CertificateTrustStore::instance().isPinned(QStringLiteral("UNPINNED-SERIAL")));
        QVERIFY(!CertificateTrustStore::instance().isTrusted(QStringLiteral("UNPINNED-SERIAL"), QStringLiteral("aa:bb")));
    }

    void pinningThenMatchingFingerprintIsTrusted()
    {
        auto &store = CertificateTrustStore::instance();
        store.trust(QStringLiteral("SERIAL-A"), QStringLiteral("11:22:33"));

        QVERIFY(store.isPinned(QStringLiteral("SERIAL-A")));
        QVERIFY(store.isTrusted(QStringLiteral("SERIAL-A"), QStringLiteral("11:22:33")));
    }

    void pinningThenDifferentFingerprintIsRejected()
    {
        auto &store = CertificateTrustStore::instance();
        store.trust(QStringLiteral("SERIAL-B"), QStringLiteral("aa:aa:aa"));

        QVERIFY(store.isPinned(QStringLiteral("SERIAL-B")));
        QVERIFY(!store.isTrusted(QStringLiteral("SERIAL-B"), QStringLiteral("bb:bb:bb")));
    }

private:
    QTemporaryDir m_configDir;
};

QTEST_MAIN(CertificateTrustStoreTest)
#include "certificatetruststoretest.moc"
