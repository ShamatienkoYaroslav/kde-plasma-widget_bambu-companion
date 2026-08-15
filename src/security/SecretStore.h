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

private:
    explicit SecretStore(QObject *parent = nullptr);

    KWallet::Wallet *wallet() const;
    static QString entryKey(const QUuid &printerId);

    mutable KWallet::Wallet *m_wallet = nullptr;
};
