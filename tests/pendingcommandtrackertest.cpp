#include <QSignalSpy>
#include <QTest>

#include "transport/lan/PendingCommandTracker.h"

class PendingCommandTrackerTest : public QObject
{
    Q_OBJECT

private slots:
    void resolvingBeforeTimeoutReportsTheGivenOutcome()
    {
        PendingCommandTracker tracker;
        QSignalSpy ackedSpy(&tracker, &PendingCommandTracker::acked);

        tracker.track(QStringLiteral("1"), 10000);
        QVERIFY(tracker.resolve(QStringLiteral("1"), true, QString()));

        QCOMPARE(ackedSpy.count(), 1);
        const QList<QVariant> args = ackedSpy.takeFirst();
        QCOMPARE(args.at(0).toString(), QStringLiteral("1"));
        QCOMPARE(args.at(1).toBool(), true);
    }

    void resolvingAnUnknownSequenceIdIsANoOp()
    {
        PendingCommandTracker tracker;
        QSignalSpy ackedSpy(&tracker, &PendingCommandTracker::acked);

        QVERIFY(!tracker.resolve(QStringLiteral("does-not-exist"), true, QString()));
        QCOMPARE(ackedSpy.count(), 0);
    }

    void unresolvedCommandTimesOutAsFailure()
    {
        PendingCommandTracker tracker;
        QSignalSpy ackedSpy(&tracker, &PendingCommandTracker::acked);

        tracker.track(QStringLiteral("2"), 50);
        QVERIFY(ackedSpy.wait(1000));

        QCOMPARE(ackedSpy.count(), 1);
        const QList<QVariant> args = ackedSpy.takeFirst();
        QCOMPARE(args.at(0).toString(), QStringLiteral("2"));
        QCOMPARE(args.at(1).toBool(), false);
        QVERIFY(!args.at(2).toString().isEmpty());
    }

    void resolvingTheSameSequenceIdTwiceOnlyAcksOnce()
    {
        PendingCommandTracker tracker;
        QSignalSpy ackedSpy(&tracker, &PendingCommandTracker::acked);

        tracker.track(QStringLiteral("3"), 10000);
        QVERIFY(tracker.resolve(QStringLiteral("3"), true, QString()));
        QVERIFY(!tracker.resolve(QStringLiteral("3"), false, QStringLiteral("late")));

        QCOMPARE(ackedSpy.count(), 1);
    }
};

QTEST_MAIN(PendingCommandTrackerTest)
#include "pendingcommandtrackertest.moc"
