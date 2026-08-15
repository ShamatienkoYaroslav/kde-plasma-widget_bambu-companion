#include "CertificateProbe.h"

#include <QCryptographicHash>
#include <QSslSocket>

CertificateProbe::CertificateProbe(QObject *parent)
    : QObject(parent)
{
}

void CertificateProbe::probe(const QString &host, quint16 port)
{
    auto *socket = new QSslSocket(this);

    // The printer's certificate is self-signed, so chain validation is
    // expected to fail; ignore it here and rely on our own fingerprint
    // pinning (CertificateTrustStore) for trust instead.
    connect(socket, &QSslSocket::sslErrors, socket, qOverload<>(&QSslSocket::ignoreSslErrors));

    connect(socket, &QSslSocket::encrypted, this, [this, socket]() {
        const QSslCertificate certificate = socket->peerCertificate();
        if (certificate.isNull()) {
            Q_EMIT probeFailed(QStringLiteral("Printer did not present a certificate"));
        } else {
            const QString fingerprint = QString::fromLatin1(certificate.digest(QCryptographicHash::Sha256).toHex(':'));
            Q_EMIT fingerprintReady(fingerprint);
        }
        socket->disconnectFromHost();
        socket->deleteLater();
    });

    connect(socket, &QSslSocket::errorOccurred, this, [this, socket](QAbstractSocket::SocketError) {
        Q_EMIT probeFailed(socket->errorString());
        socket->deleteLater();
    });

    socket->connectToHostEncrypted(host, port);
}
