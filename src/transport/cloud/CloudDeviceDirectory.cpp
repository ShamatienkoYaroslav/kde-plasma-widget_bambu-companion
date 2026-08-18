#include "CloudDeviceDirectory.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "CloudAuthClient.h"

namespace
{
constexpr auto kBoundDevicesUrl = "https://api.bambulab.com/v1/iot-service/api/user/bind";
}

CloudDeviceDirectory::CloudDeviceDirectory(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void CloudDeviceDirectory::fetchDevices()
{
    QNetworkRequest request{QUrl(QString::fromLatin1(kBoundDevicesUrl))};
    request.setRawHeader("Authorization", "Bearer " + CloudAuthClient::instance().accessToken().toUtf8());

    auto *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
        reply->deleteLater();
    });
}

void CloudDeviceDirectory::onReplyFinished(QNetworkReply *reply)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();

    CloudAuthClient::instance().handleHttpStatus(status);

    if (status < 200 || status >= 300) {
        Q_EMIT fetchFailed(QStringLiteral("Failed to fetch printers (HTTP %1)").arg(status));
        return;
    }

    // Diagnostic, kept intentionally (not "temporary"): confirmed against a
    // real account that no account-level uid appears anywhere in this
    // response either ("code"/"devices"/"error"/"message" at the top; each
    // device entry has dev_id/dev_access_code/name/online/etc., no uid) —
    // see CloudAuthClient.cpp's matching note and design.md's Risks. Logs
    // only key names, never values.
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    qWarning() << "CloudDeviceDirectory: bind response top-level keys:" << root.keys();
    const QJsonArray devicesArray = root.value(QStringLiteral("devices")).toArray();
    if (!devicesArray.isEmpty()) {
        qWarning() << "CloudDeviceDirectory: first device entry keys:" << devicesArray.first().toObject().keys();
    }

    Q_EMIT devicesReady(parseDevices(body));
}

QList<CloudDeviceInfo> CloudDeviceDirectory::parseDevices(const QByteArray &body)
{
    QList<CloudDeviceInfo> devices;

    const QJsonObject root = QJsonDocument::fromJson(body).object();
    const QJsonArray list = root.value(QStringLiteral("devices")).toArray();
    for (const QJsonValue &value : list) {
        const QJsonObject entry = value.toObject();
        CloudDeviceInfo info;
        info.devId = entry.value(QStringLiteral("dev_id")).toString();
        info.name = entry.value(QStringLiteral("name")).toString();
        info.online = entry.value(QStringLiteral("online")).toBool();
        if (!info.devId.isEmpty()) {
            devices.append(info);
        }
    }

    return devices;
}
