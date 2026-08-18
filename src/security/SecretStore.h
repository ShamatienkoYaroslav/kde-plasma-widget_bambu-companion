#pragma once

#include <QObject>
#include <QString>
#include <QUuid>

namespace KWallet
{
class Wallet;
}

class SecretStore : public QObject
{
    Q_OBJECT

public:
    static SecretStore &instance();

    void storeLanAccessCode(const QUuid &printerId, const QString &accessCode);
    QString lanAccessCode(const QUuid &printerId) const;
    void removeLanAccessCode(const QUuid &printerId);

    // Single Bambu Cloud account token — no per-printer scoping (see design.md).
    void storeCloudToken(const QString &token);
    QString cloudToken() const;
    void removeCloudToken();

private:
    explicit SecretStore(QObject *parent = nullptr);

    KWallet::Wallet *wallet() const;
    static QString entryKey(const QUuid &printerId);

    mutable KWallet::Wallet *m_wallet = nullptr;
};
