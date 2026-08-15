#pragma once

#include <QObject>
#include <QString>

class CertificateProbe : public QObject
{
    Q_OBJECT

public:
    explicit CertificateProbe(QObject *parent = nullptr);

    void probe(const QString &host, quint16 port);

Q_SIGNALS:
    void fingerprintReady(const QString &sha256Fingerprint);
    void probeFailed(const QString &errorString);
};
