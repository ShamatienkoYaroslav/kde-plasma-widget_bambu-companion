#pragma once

#include <QString>

class CertificateTrustStore
{
public:
    static CertificateTrustStore &instance();

    bool isPinned(const QString &serial) const;
    bool isTrusted(const QString &serial, const QString &fingerprint) const;
    void trust(const QString &serial, const QString &fingerprint);

private:
    CertificateTrustStore() = default;
};
