#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "core/PrinterStatus.h"
#include "protocol/BambuReportParser.h"

namespace
{
QJsonObject loadFixture(const QString &name)
{
    QFile file(QStringLiteral(FIXTURES_DIR "/") + name);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}
}

class BambuReportParserTest : public QObject
{
    Q_OBJECT

private slots:
    void fullReportParsesCompletely()
    {
        const QJsonObject report = loadFixture(QStringLiteral("full_report.json"));
        QVERIFY(!report.isEmpty());

        const PrinterStatus status = BambuReportParser::merge(PrinterStatus(), report);

        QCOMPARE(status.state, PrinterStatus::State::Printing);
        QCOMPARE(status.progressPercent.value_or(-1), 42);
        QCOMPARE(status.currentLayer.value_or(-1), 120);
        QCOMPARE(status.totalLayers.value_or(-1), 280);
        QCOMPARE(status.nozzleTemp.value_or(-1.0), 219.8);
        QCOMPARE(status.nozzleTargetTemp.value_or(-1.0), 220.0);
        QCOMPARE(status.bedTemp.value_or(-1.0), 59.6);
        QCOMPARE(status.chamberTemp.value_or(-1.0), 34.0);
        QCOMPARE(status.coolingFanPercent.value_or(-1), 100);
        QCOMPARE(status.speedProfile.value_or(QString()), QStringLiteral("Standard"));
        QCOMPARE(status.wifiSignalDbm.value_or(0), -53);
    }

    void deltaReportUpdatesOnlyItsFields()
    {
        const QJsonObject full = loadFixture(QStringLiteral("full_report.json"));
        const QJsonObject delta = loadFixture(QStringLiteral("delta_report.json"));
        QVERIFY(!full.isEmpty());
        QVERIFY(!delta.isEmpty());

        const PrinterStatus afterFull = BambuReportParser::merge(PrinterStatus(), full);
        const PrinterStatus afterDelta = BambuReportParser::merge(afterFull, delta);

        // The field present in the delta changes...
        QCOMPARE(afterDelta.nozzleTemp.value_or(-1.0), 221.4);

        // ...everything else carries forward unchanged from the full report.
        QCOMPARE(afterDelta.state, afterFull.state);
        QCOMPARE(afterDelta.progressPercent.value_or(-1), afterFull.progressPercent.value_or(-1));
        QCOMPARE(afterDelta.bedTemp.value_or(-1.0), afterFull.bedTemp.value_or(-1.0));
        QCOMPARE(afterDelta.currentLayer.value_or(-1), afterFull.currentLayer.value_or(-1));
    }
};

QTEST_MAIN(BambuReportParserTest)
#include "bambureportparsertest.moc"
