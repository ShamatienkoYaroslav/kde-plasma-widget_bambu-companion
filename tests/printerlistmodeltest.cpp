#include <QTest>

#include "PrinterListModel.h"

class PrinterListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void hasPlaceholderRows()
    {
        PrinterListModel model;
        QCOMPARE(model.rowCount(), 3);

        const QModelIndex first = model.index(0, 0);
        QVERIFY(first.isValid());
        QVERIFY(!model.data(first, PrinterListModel::NameRole).toString().isEmpty());
        QVERIFY(!model.data(first, PrinterListModel::StatusRole).toString().isEmpty());
    }
};

QTEST_MAIN(PrinterListModelTest)
#include "printerlistmodeltest.moc"
