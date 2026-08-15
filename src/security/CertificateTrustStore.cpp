#include "CertificateTrustStore.h"

#include <KConfigGroup>
#include <KSharedConfig>

namespace
{
KConfigGroup trustGroup()
{
    // Same explicit filename as PrinterRegistry, so both live in the applet's
    // own config file rather than the host process's default config.
    return KSharedConfig::openConfig(QStringLiteral("bambucompanionrc"))->group(QStringLiteral("CertificateTrust"));
}
}

CertificateTrustStore &CertificateTrustStore::instance()
{
    static CertificateTrustStore store;
    return store;
}

bool CertificateTrustStore::isPinned(const QString &serial) const
{
    return trustGroup().hasKey(serial);
}

bool CertificateTrustStore::isTrusted(const QString &serial, const QString &fingerprint) const
{
    return trustGroup().readEntry(serial) == fingerprint;
}

void CertificateTrustStore::trust(const QString &serial, const QString &fingerprint)
{
    KConfigGroup group = trustGroup();
    group.writeEntry(serial, fingerprint);
    group.sync();
}
