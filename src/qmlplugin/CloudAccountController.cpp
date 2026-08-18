#include "CloudAccountController.h"

#include "../transport/cloud/CloudAuthClient.h"

CloudAccountController::CloudAccountController(QObject *parent)
    : QObject(parent)
{
    auto &auth = CloudAuthClient::instance();
    connect(&auth, &CloudAuthClient::loginSucceeded, this, &CloudAccountController::loginSucceeded);
    connect(&auth, &CloudAuthClient::loginSucceeded, this, &CloudAccountController::loggedInChanged);
    connect(&auth, &CloudAuthClient::twoFactorRequired, this, &CloudAccountController::twoFactorRequired);
    connect(&auth, &CloudAuthClient::loginFailed, this, &CloudAccountController::loginFailed);
    connect(&auth, &CloudAuthClient::loggedOut, this, &CloudAccountController::loggedOut);
    connect(&auth, &CloudAuthClient::loggedOut, this, &CloudAccountController::loggedInChanged);
}

void CloudAccountController::login(const QString &email, const QString &password)
{
    CloudAuthClient::instance().login(email, password);
}

void CloudAccountController::submitVerificationCode(const QString &code)
{
    CloudAuthClient::instance().submitVerificationCode(code);
}

void CloudAccountController::logout()
{
    CloudAuthClient::instance().logout();
}

bool CloudAccountController::isLoggedIn() const
{
    return CloudAuthClient::instance().isLoggedIn();
}
