#include <QTemporaryDir>
#include <QTest>

#include "qmlplugin/PrinterController.h"
#include "qmlplugin/PrinterListModel.h"

class PrinterListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_configDir.isValid());
        qputenv("XDG_CONFIG_HOME", m_configDir.path().toUtf8());
    }

    void reflectsAddedPrinter()
    {
        PrinterListModel model;
        QCOMPARE(model.rowCount(), 0);

        PrinterController controller;
        controller.addLanPrinter(QStringLiteral("Test Printer"), QStringLiteral("203.0.113.1"), QStringLiteral("TEST0001"), QStringLiteral("secret"), 8883);

        QCOMPARE(model.rowCount(), 1);
        const QModelIndex idx = model.index(0, 0);
        QVERIFY(idx.isValid());
        QCOMPARE(model.data(idx, PrinterListModel::NameRole).toString(), QStringLiteral("Test Printer"));
    }

private:
    QTemporaryDir m_configDir;
};

QTEST_MAIN(PrinterListModelTest)
#include "printerlistmodeltest.moc"
