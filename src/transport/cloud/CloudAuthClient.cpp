#include "CloudAuthClient.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

#include "../../security/SecretStore.h"

namespace
{
constexpr auto kLoginUrl = "https://api.bambulab.com/v1/user-service/user/login";
constexpr auto kSendCodeUrl = "https://api.bambulab.com/v1/user-service/user/sendemail/code";
constexpr auto kMyInfoUrl = "https://api.bambulab.com/v1/user-service/my/info";
}

CloudAuthClient &CloudAuthClient::instance()
{
    static CloudAuthClient client;
    return client;
}

CloudAuthClient::CloudAuthClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

bool CloudAuthClient::isLoggedIn() const
{
    return !SecretStore::instance().cloudToken().isEmpty();
}

QString CloudAuthClient::accessToken() const
{
    return SecretStore::instance().cloudToken();
}

QString CloudAuthClient::cachedUserId() const
{
    return m_cachedUserId;
}

void CloudAuthClient::ensureUserId()
{
    if (!m_cachedUserId.isEmpty()) {
        Q_EMIT userIdReady(m_cachedUserId);
        return;
    }
    if (m_userIdFetchInFlight || !isLoggedIn()) {
        return;
    }

    m_userIdFetchInFlight = true;

    QNetworkRequest request{QUrl(QString::fromLatin1(kMyInfoUrl))};
    request.setRawHeader("Authorization", "Bearer " + accessToken().toUtf8());

    auto *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onUserInfoReplyFinished(reply);
        reply->deleteLater();
    });
}

void CloudAuthClient::onUserInfoReplyFinished(QNetworkReply *reply)
{
    m_userIdFetchInFlight = false;

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();

    logDiagnostic(QStringLiteral("my-info"), status, body);
    // Deliberately NOT calling handleHttpStatus() here: the exact endpoint
    // for this request is itself an unconfirmed guess (see the diagnostic
    // below), so a wrong/erroring endpoint must not be able to invalidate an
    // otherwise-valid session — only a 401 from an endpoint already proven
    // correct (e.g. CloudDeviceDirectory's device list) should trigger that.

    const QJsonObject root = QJsonDocument::fromJson(body).object();
    // Diagnostic, kept intentionally (not "temporary"): kMyInfoUrl is
    // confirmed WRONG (404) against a real account as of the Phase 3 manual
    // verification pass — neither the login response nor the device-list
    // response (see CloudDeviceDirectory) carry an account uid either, so
    // the "u_<uid>" cloud MQTT username scheme itself is unconfirmed, not
    // just this lookup. Live cloud MQTT status is a known, documented gap
    // until the correct source for the uid (or the correct username scheme
    // entirely) is found — see design.md's Risks. Logs only key names,
    // never values.
    qWarning() << "CloudAuthClient: my/info HTTP" << status << "response keys:" << root.keys();

    QString uid;
    for (const auto &key : {QStringLiteral("uid"), QStringLiteral("id"), QStringLiteral("accountId")}) {
        const QJsonValue value = root.value(key);
        if (value.isString()) {
            uid = value.toString();
            break;
        }
        if (value.isDouble()) {
            uid = QString::number(value.toDouble(), 'f', 0);
            break;
        }
    }

    if (uid.isEmpty()) {
        Q_EMIT userIdFetchFailed(QStringLiteral("Could not determine account user id (HTTP %1)").arg(status));
        return;
    }

    m_cachedUserId = uid;
    Q_EMIT userIdReady(uid);
}

void CloudAuthClient::login(const QString &email, const QString &password)
{
    m_pendingEmail = email;

    QJsonObject body;
    body.insert(QStringLiteral("account"), email);
    body.insert(QStringLiteral("password"), password);
    sendLoginRequest(body);
}

void CloudAuthClient::submitVerificationCode(const QString &code)
{
    QJsonObject body;
    body.insert(QStringLiteral("account"), m_pendingEmail);
    body.insert(QStringLiteral("code"), code);
    sendLoginRequest(body);
}

void CloudAuthClient::logout()
{
    SecretStore::instance().removeCloudToken();
    m_cachedUserId.clear();
    Q_EMIT loggedOut();
}

void CloudAuthClient::sendLoginRequest(const QJsonObject &body)
{
    QNetworkRequest request{QUrl(QString::fromLatin1(kLoginUrl))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    auto *reply = m_networkManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onLoginReplyFinished(reply);
        reply->deleteLater();
    });
}

void CloudAuthClient::onLoginReplyFinished(QNetworkReply *reply)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();

    logDiagnostic(QStringLiteral("login"), status, body);

    const ParsedLoginResponse parsed = parseLoginResponse(status, body);
    switch (parsed.outcome) {
    case LoginOutcome::Success:
        SecretStore::instance().storeCloudToken(parsed.token);
        Q_EMIT loginSucceeded();
        break;
    case LoginOutcome::VerificationRequired:
        requestVerificationEmail();
        Q_EMIT twoFactorRequired();
        break;
    case LoginOutcome::Failure:
        Q_EMIT loginFailed(parsed.failureReason);
        break;
    }
}

void CloudAuthClient::requestVerificationEmail()
{
    QJsonObject body;
    body.insert(QStringLiteral("email"), m_pendingEmail);
    body.insert(QStringLiteral("type"), QStringLiteral("codeLogin"));

    QNetworkRequest request{QUrl(QString::fromLatin1(kSendCodeUrl))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    auto *reply = m_networkManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

CloudAuthClient::ParsedLoginResponse CloudAuthClient::parseLoginResponse(int httpStatusCode, const QByteArray &body)
{
    ParsedLoginResponse result;

    const QJsonObject root = QJsonDocument::fromJson(body).object();

    const QString accessToken = root.value(QStringLiteral("accessToken")).toString();
    if (httpStatusCode >= 200 && httpStatusCode < 300 && !accessToken.isEmpty()) {
        result.outcome = LoginOutcome::Success;
        result.token = accessToken;
        return result;
    }

    const QString loginType = root.value(QStringLiteral("loginType")).toString();
    if (loginType.compare(QStringLiteral("verifyCode"), Qt::CaseInsensitive) == 0) {
        result.outcome = LoginOutcome::VerificationRequired;
        return result;
    }

    result.outcome = LoginOutcome::Failure;
    const QString message = root.value(QStringLiteral("error")).toString();
    result.failureReason = !message.isEmpty() ? message : QStringLiteral("Login failed (HTTP %1)").arg(httpStatusCode);
    return result;
}

void CloudAuthClient::handleHttpStatus(int httpStatusCode)
{
    if (httpStatusCode == 401) {
        SecretStore::instance().removeCloudToken();
        m_cachedUserId.clear();
        Q_EMIT loggedOut();
    }
}

void CloudAuthClient::logDiagnostic(const QString &context, int httpStatus, const QByteArray &body) const
{
    QJsonObject redacted = QJsonDocument::fromJson(body).object();
    for (const auto &key : {QStringLiteral("accessToken"), QStringLiteral("refreshToken"), QStringLiteral("password"), QStringLiteral("code")}) {
        if (redacted.contains(key)) {
            redacted[key] = QStringLiteral("<redacted>");
        }
    }

    const QString logPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/cloud-diagnostic.log");
    QFile file(logPath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [" << context << "] HTTP " << httpStatus << ' '
               << QString::fromUtf8(QJsonDocument(redacted).toJson(QJsonDocument::Compact)) << '\n';
    }
}
