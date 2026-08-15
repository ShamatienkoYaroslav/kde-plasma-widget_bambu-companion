#pragma once

#include <QHash>
#include <QObject>
#include <QString>

class QTimer;

// Tracks in-flight command sequence ids and their timeouts, independent of
// any actual transport — kept separate from LanPrinterConnection so the
// sequence-id/timeout matching logic is testable without a real MQTT/network
// connection.
class PendingCommandTracker : public QObject
{
    Q_OBJECT

public:
    explicit PendingCommandTracker(QObject *parent = nullptr);

    // Registers a newly sent command's sequence id and starts its timeout;
    // acked() fires with success=false if resolve() isn't called before then.
    void track(const QString &sequenceId, int timeoutMs);

    // Resolves a pending sequence id (from either a real ack or a timeout).
    // Returns false (and does nothing else) if the id isn't pending.
    bool resolve(const QString &sequenceId, bool success, const QString &reason);

Q_SIGNALS:
    void acked(const QString &sequenceId, bool success, const QString &reason);

private:
    QHash<QString, QTimer *> m_pending;
};
