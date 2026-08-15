#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <qqmlregistration.h>

class PrinterController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit PrinterController(QObject *parent = nullptr);

    Q_INVOKABLE void addLanPrinter(const QString &name, const QString &host, const QString &serial, const QString &accessCode, int mqttPort);
    Q_INVOKABLE void removePrinter(const QString &printerId);
    Q_INVOKABLE void confirmCertificateTrust(const QString &printerId, const QString &fingerprint, bool accept);

    Q_INVOKABLE void pause(const QString &printerId);
    Q_INVOKABLE void resume(const QString &printerId);
    Q_INVOKABLE void stop(const QString &printerId);
    Q_INVOKABLE void skipObjects(const QString &printerId, const QList<int> &objectIds);

Q_SIGNALS:
    void certificateTrustNeeded(const QString &printerId, const QString &printerName, const QString &fingerprint);
    void commandFailed(const QString &printerId, const QString &reason);
};
