#include "CloudAccountController.h"

#include <QVariantMap>

#include "../core/PrinterRegistry.h"
#include "../transport/cloud/CloudAuthClient.h"
#include "../transport/cloud/CloudDeviceDirectory.h"

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

void CloudAccountController::fetchDevices()
{
    // A fresh directory per fetch, self-discarding once the request settles.
    auto *directory = new CloudDeviceDirectory(this);

    connect(directory, &CloudDeviceDirectory::devicesReady, this, [this, directory](const QList<CloudDeviceInfo> &devices) {
        QVariantList list;
        for (const CloudDeviceInfo &device : devices) {
            QVariantMap entry;
            entry[QStringLiteral("devId")] = device.devId;
            entry[QStringLiteral("name")] = device.name;
            entry[QStringLiteral("online")] = device.online;
            list.append(entry);
        }
        Q_EMIT devicesReady(list);
        directory->deleteLater();
    });

    connect(directory, &CloudDeviceDirectory::fetchFailed, this, [this, directory](const QString &reason) {
        Q_EMIT fetchFailed(reason);
        directory->deleteLater();
    });

    directory->fetchDevices();
}

void CloudAccountController::addCloudPrinter(const QString &devId, const QString &name)
{
    PrinterRegistry::instance().addCloudPrinter(devId, name);
}
