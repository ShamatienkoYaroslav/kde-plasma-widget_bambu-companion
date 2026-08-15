#pragma once

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

Q_SIGNALS:
    void certificateTrustNeeded(const QString &printerId, const QString &printerName, const QString &fingerprint);
};
