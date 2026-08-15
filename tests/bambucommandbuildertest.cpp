#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "protocol/BambuCommandBuilder.h"

class BambuCommandBuilderTest : public QObject
{
    Q_OBJECT

private slots:
    void pauseProducesExpectedShape()
    {
        const QJsonObject print = QJsonDocument::fromJson(BambuCommandBuilder::pause(QStringLiteral("7"))).object().value(QStringLiteral("print")).toObject();
        QCOMPARE(print.value(QStringLiteral("command")).toString(), QStringLiteral("pause"));
        QCOMPARE(print.value(QStringLiteral("sequence_id")).toString(), QStringLiteral("7"));
    }

    void resumeProducesExpectedShape()
    {
        const QJsonObject print = QJsonDocument::fromJson(BambuCommandBuilder::resume(QStringLiteral("8"))).object().value(QStringLiteral("print")).toObject();
        QCOMPARE(print.value(QStringLiteral("command")).toString(), QStringLiteral("resume"));
        QCOMPARE(print.value(QStringLiteral("sequence_id")).toString(), QStringLiteral("8"));
    }

    void stopProducesExpectedShape()
    {
        const QJsonObject print = QJsonDocument::fromJson(BambuCommandBuilder::stop(QStringLiteral("9"))).object().value(QStringLiteral("print")).toObject();
        QCOMPARE(print.value(QStringLiteral("command")).toString(), QStringLiteral("stop"));
        QCOMPARE(print.value(QStringLiteral("sequence_id")).toString(), QStringLiteral("9"));
    }

    void skipObjectsProducesExpectedShape()
    {
        const QJsonObject print =
            QJsonDocument::fromJson(BambuCommandBuilder::skipObjects(QStringLiteral("10"), {1, 2, 3})).object().value(QStringLiteral("print")).toObject();
        QCOMPARE(print.value(QStringLiteral("command")).toString(), QStringLiteral("skip_objects"));
        QCOMPARE(print.value(QStringLiteral("sequence_id")).toString(), QStringLiteral("10"));
        QVERIFY(print.contains(QStringLiteral("timestamp")));

        const QJsonArray objList = print.value(QStringLiteral("obj_list")).toArray();
        QCOMPARE(objList.size(), 3);
        QCOMPARE(objList.at(0).toInt(), 1);
        QCOMPARE(objList.at(1).toInt(), 2);
        QCOMPARE(objList.at(2).toInt(), 3);
    }
};

QTEST_MAIN(BambuCommandBuilderTest)
#include "bambucommandbuildertest.moc"
