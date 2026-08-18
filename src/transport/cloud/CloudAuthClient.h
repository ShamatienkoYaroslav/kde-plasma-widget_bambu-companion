#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// Single Bambu Cloud account login/session state (see design.md — no
// multi-account support). A singleton, like PrinterRegistry/SecretStore,
// since CloudDeviceDirectory and CloudPrinterConnection both need to share
// the one account's login state.
class CloudAuthClient : public QObject
{
    Q_OBJECT

public:
    enum class LoginOutcome {
        Success,
        VerificationRequired,
        Failure,
    };

    struct ParsedLoginResponse {
        LoginOutcome outcome = LoginOutcome::Failure;
        QString token; // valid when outcome == Success
        QString failureReason; // valid when outcome == Failure
    };

    static CloudAuthClient &instance();

    void login(const QString &email, const QString &password);
    void submitVerificationCode(const QString &code);
    void logout();

    bool isLoggedIn() const;
    QString accessToken() const;

    // The account's numeric user id, needed for the cloud MQTT relay's
    // username ("u_<id>"). The access token turned out to be an opaque
    // string, not a JWT (an earlier assumption in this class, since
    // corrected) — there's no way to derive this locally, so it's fetched
    // from a "my account info" endpoint and cached in memory for the
    // process's lifetime once known.
    QString cachedUserId() const;
    // Triggers a fetch if not already cached/in flight; safe to call
    // repeatedly (e.g. once per CloudPrinterConnection on startup — they
    // all share this one singleton's in-flight request and result).
    void ensureUserId();

    // Pure response-parsing logic, exposed so it's testable without a real
    // network call or a mocked QNetworkAccessManager.
    static ParsedLoginResponse parseLoginResponse(int httpStatusCode, const QByteArray &body);

    // Called by any cloud transport (this class, CloudDeviceDirectory,
    // CloudPrinterConnection) whenever an authenticated HTTP call's status is
    // known; clears the stored token and emits loggedOut() on 401, rather
    // than guessing at an undocumented refresh flow. A no-op for any other
    // status. Exposed as its own method so the "what happens on 401" logic
    // is directly testable without a real network call.
    void handleHttpStatus(int httpStatusCode);

Q_SIGNALS:
    void loginSucceeded();
    void twoFactorRequired();
    void loginFailed(const QString &reason);
    void loggedOut();
    void userIdReady(const QString &userId);
    void userIdFetchFailed(const QString &reason);

private:
    explicit CloudAuthClient(QObject *parent = nullptr);

    void sendLoginRequest(const QJsonObject &body);
    void requestVerificationEmail();
    void onLoginReplyFinished(QNetworkReply *reply);
    void onUserInfoReplyFinished(QNetworkReply *reply);
    void logDiagnostic(const QString &context, int httpStatus, const QByteArray &body) const;

    QNetworkAccessManager *m_networkManager = nullptr;
    QString m_pendingEmail;
    QString m_cachedUserId;
    bool m_userIdFetchInFlight = false;
};
