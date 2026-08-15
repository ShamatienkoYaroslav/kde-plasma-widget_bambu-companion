#include "SecretStore.h"

#include <kwallet.h>

namespace
{
const char kWalletFolder[] = "BambuCompanion";
}

SecretStore &SecretStore::instance()
{
    static SecretStore store;
    return store;
}

SecretStore::SecretStore(QObject *parent)
    : QObject(parent)
{
}

KWallet::Wallet *SecretStore::wallet() const
{
    // Opened synchronously (blocking) rather than via KWallet's async API:
    // callers (PrinterRegistry::addLanPrinter) need the result immediately
    // and the wallet is typically already unlocked via PAM integration, so
    // the added state-machine complexity of the async API isn't worth it here.
    if (!m_wallet) {
        m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous);
        if (m_wallet) {
            if (!m_wallet->hasFolder(QLatin1String(kWalletFolder))) {
                m_wallet->createFolder(QLatin1String(kWalletFolder));
            }
            m_wallet->setFolder(QLatin1String(kWalletFolder));
        }
    }
    return m_wallet;
}

QString SecretStore::entryKey(const QUuid &printerId)
{
    return QStringLiteral("lan-access-code/%1").arg(printerId.toString());
}

void SecretStore::storeLanAccessCode(const QUuid &printerId, const QString &accessCode)
{
    if (auto *w = wallet()) {
        w->writePassword(entryKey(printerId), accessCode);
    }
}

void SecretStore::removeLanAccessCode(const QUuid &printerId)
{
    if (auto *w = wallet()) {
        w->removeEntry(entryKey(printerId));
    }
}

QString SecretStore::lanAccessCode(const QUuid &printerId) const
{
    if (auto *w = wallet()) {
        QString password;
        if (w->readPassword(entryKey(printerId), password) == 0) {
            return password;
        }
    }
    return QString();
}
