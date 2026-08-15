#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QUuid>

#include "PrinterConnection.h"
#include "PrinterProfile.h"
#include "PrinterStatus.h"

class PrinterRegistry : public QObject
{
    Q_OBJECT

public:
    static PrinterRegistry &instance();

    QUuid addLanPrinter(const QString &name, const QString &host, const QString &serial, const QString &accessCode, quint16 mqttPort = 8883);
    void removePrinter(const QUuid &id);

    QList<PrinterProfile> printers() const;
    PrinterProfile profile(const QUuid &id) const;
    PrinterStatus status(const QUuid &id) const;
    PrinterConnection::ConnectionState connectionState(const QUuid &id) const;

    void confirmCertificateTrust(const QUuid &id, const QString &fingerprint, bool accept);

Q_SIGNALS:
    void printerAdded(const QUuid &id);
    void printerRemoved(const QUuid &id);
    void printerStatusChanged(const QUuid &id);
    void printerConnectionStateChanged(const QUuid &id);
    void certificateTrustNeeded(const QUuid &id, const QString &fingerprint);

private:
    explicit PrinterRegistry(QObject *parent = nullptr);

    void loadPersistedPrinters();
    void persistProfile(const PrinterProfile &profile);
    void startConnection(const PrinterProfile &profile);

    QHash<QUuid, PrinterProfile> m_profiles;
    QHash<QUuid, PrinterConnection *> m_connections;
    QHash<QUuid, PrinterStatus> m_statuses;
};
