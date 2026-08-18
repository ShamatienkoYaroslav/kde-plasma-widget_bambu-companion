#include <QTest>

#include "transport/cloud/CloudAuthClient.h"

class CloudAuthClientTest : public QObject
{
    Q_OBJECT

private slots:
    void successfulResponseParsesTheToken()
    {
        const QByteArray body = R"({"accessToken":"abc123","refreshToken":"xyz"})";
        const auto result = CloudAuthClient::parseLoginResponse(200, body);

        QCOMPARE(result.outcome, CloudAuthClient::LoginOutcome::Success);
        QCOMPARE(result.token, QStringLiteral("abc123"));
    }

    void verifyCodeResponseRequestsVerification()
    {
        const QByteArray body = R"({"loginType":"verifyCode"})";
        const auto result = CloudAuthClient::parseLoginResponse(200, body);

        QCOMPARE(result.outcome, CloudAuthClient::LoginOutcome::VerificationRequired);
    }

    void errorResponseReportsFailureWithAMessage()
    {
        const QByteArray body = R"({"error":"Invalid account or password"})";
        const auto result = CloudAuthClient::parseLoginResponse(400, body);

        QCOMPARE(result.outcome, CloudAuthClient::LoginOutcome::Failure);
        QCOMPARE(result.failureReason, QStringLiteral("Invalid account or password"));
    }

    void unparseableResponseStillReportsFailure()
    {
        const auto result = CloudAuthClient::parseLoginResponse(500, QByteArray());

        QCOMPARE(result.outcome, CloudAuthClient::LoginOutcome::Failure);
        QVERIFY(!result.failureReason.isEmpty());
    }

    void unauthorizedStatusAlwaysLeavesTheAccountLoggedOut()
    {
        // Doesn't assert a prior "logged in" state, since that depends on a
        // working KWallet (see printerregistrytest.cpp's equivalent note) —
        // but the post-condition holds regardless of whether the underlying
        // store is available.
        CloudAuthClient::instance().handleHttpStatus(401);
        QVERIFY(!CloudAuthClient::instance().isLoggedIn());
    }
};

QTEST_MAIN(CloudAuthClientTest)
#include "cloudauthclienttest.moc"
