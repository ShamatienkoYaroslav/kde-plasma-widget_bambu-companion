#pragma once

#include <QList>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

struct CloudDeviceInfo {
    QString devId;
    QString name;
    bool online = false;
};

// Fetches the logged-in Bambu Cloud account's bound device list.
class CloudDeviceDirectory : public QObject
{
    Q_OBJECT

public:
    explicit CloudDeviceDirectory(QObject *parent = nullptr);

    void fetchDevices();

    // Pure parsing logic, exposed for testing without a real network call.
    static QList<CloudDeviceInfo> parseDevices(const QByteArray &body);

Q_SIGNALS:
    void devicesReady(const QList<CloudDeviceInfo> &devices);
    void fetchFailed(const QString &reason);

private:
    void onReplyFinished(QNetworkReply *reply);

    QNetworkAccessManager *m_networkManager = nullptr;
};
