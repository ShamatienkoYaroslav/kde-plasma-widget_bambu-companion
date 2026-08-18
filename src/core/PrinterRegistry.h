#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QUuid>

#include "PrinterConnection.h"
#include "PrinterProfile.h"
#include "PrinterStatus.h"

class PrinterRegistry : public QObject
{
    Q_OBJECT

public:
    static PrinterRegistry &instance();

    // `mode` defaults to LanOnly; PreferLanThenCloud is reachable through
    // this same entry point (no separate UI path yet — see PLAN.md's Phase 3
    // scope) so the fallback behavior in printer-management's spec is at
    // least programmatically usable and testable.
    QUuid addLanPrinter(const QString &name,
                         const QString &host,
                         const QString &serial,
                         const QString &accessCode,
                         quint16 mqttPort = 8883,
                         PrinterProfile::ConnectionMode mode = PrinterProfile::ConnectionMode::LanOnly);
    QUuid addCloudPrinter(const QString &devId, const QString &name);
    void removePrinter(const QUuid &id);

    QList<PrinterProfile> printers() const;
    PrinterProfile profile(const QUuid &id) const;
    PrinterStatus status(const QUuid &id) const;
    PrinterConnection::ConnectionState connectionState(const QUuid &id) const;

    void confirmCertificateTrust(const QUuid &id, const QString &fingerprint, bool accept);
    void sendCommand(const QUuid &id, const PrinterCommand &command);

Q_SIGNALS:
    void printerAdded(const QUuid &id);
    void printerRemoved(const QUuid &id);
    void printerStatusChanged(const QUuid &id);
    void printerConnectionStateChanged(const QUuid &id);
    void certificateTrustNeeded(const QUuid &id, const QString &fingerprint);
    void commandFailed(const QUuid &id, const QString &reason);

private:
    explicit PrinterRegistry(QObject *parent = nullptr);

    void loadPersistedPrinters();
    void persistProfile(const PrinterProfile &profile);
    void startConnection(const PrinterProfile &profile);
    void wireAndStart(const QUuid &id, PrinterProfile::ConnectionMode mode, PrinterConnection *connection);
    void fallBackToCloud(const QUuid &id);

    QHash<QUuid, PrinterProfile> m_profiles;
    QHash<QUuid, PrinterConnection *> m_connections;
    QHash<QUuid, PrinterStatus> m_statuses;
    QSet<QUuid> m_cloudFallbackDone;
};
