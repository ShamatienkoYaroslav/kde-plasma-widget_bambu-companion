#include "PendingCommandTracker.h"

#include <QTimer>

PendingCommandTracker::PendingCommandTracker(QObject *parent)
    : QObject(parent)
{
}

void PendingCommandTracker::track(const QString &sequenceId, int timeoutMs)
{
    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [this, sequenceId]() {
        resolve(sequenceId, false, QStringLiteral("Timed out waiting for acknowledgement"));
    });
    m_pending.insert(sequenceId, timer);
    timer->start(timeoutMs);
}

bool PendingCommandTracker::resolve(const QString &sequenceId, bool success, const QString &reason)
{
    const auto it = m_pending.find(sequenceId);
    if (it == m_pending.end()) {
        return false;
    }
    it.value()->stop();
    it.value()->deleteLater();
    m_pending.erase(it);
    Q_EMIT acked(sequenceId, success, reason);
    return true;
}
