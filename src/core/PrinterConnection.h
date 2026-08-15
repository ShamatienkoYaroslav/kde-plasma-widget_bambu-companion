#pragma once

#include <QObject>
#include <QString>

#include "PrinterStatus.h"

class CameraSource; // Extension point implemented in Phase 4.

class PrinterConnection : public QObject
{
    Q_OBJECT

public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        AwaitingCertificateTrust,
        Connected,
        Error,
    };

    explicit PrinterConnection(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ConnectionState connectionState() const = 0;
    virtual PrinterStatus status() const = 0;

    // LAN-specific trust-on-first-use flow; no-op for transports (e.g. Cloud)
    // that don't need certificate pinning.
    virtual void confirmCertificateTrust(const QString &fingerprint, bool accept)
    {
        Q_UNUSED(fingerprint);
        Q_UNUSED(accept);
    }

    // Extension point for Phase 4's camera stream; unimplemented transports
    // simply have no camera.
    virtual CameraSource *cameraSource()
    {
        return nullptr;
    }

Q_SIGNALS:
    void connectionStateChanged(ConnectionState state);
    void statusUpdated(const PrinterStatus &status);
    void certificateTrustNeeded(const QString &fingerprint);
};
