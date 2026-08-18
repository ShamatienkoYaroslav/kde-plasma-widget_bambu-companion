#pragma once

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

class CloudAccountController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // A plain Q_INVOKABLE isLoggedIn() wouldn't be reactive in QML bindings
    // (invokable calls aren't tracked as binding dependencies) — exposed as
    // a NOTIFYing property instead so e.g. `visible: CloudAccountController.loggedIn`
    // updates when login state changes.
    Q_PROPERTY(bool loggedIn READ isLoggedIn NOTIFY loggedInChanged)

public:
    explicit CloudAccountController(QObject *parent = nullptr);

    Q_INVOKABLE void login(const QString &email, const QString &password);
    Q_INVOKABLE void submitVerificationCode(const QString &code);
    Q_INVOKABLE void logout();
    bool isLoggedIn() const;

Q_SIGNALS:
    void loginSucceeded();
    void twoFactorRequired();
    void loginFailed(const QString &reason);
    void loggedOut();
    void loggedInChanged();
};
