import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import io.github.shamatienkoyaroslav.bambucompanion

Kirigami.OverlaySheet {
    id: root

    property bool verificationStep: false

    title: i18n("Log in to Bambu Cloud")

    function reset() {
        emailField.text = "";
        passwordField.text = "";
        codeField.text = "";
        errorLabel.text = "";
        root.verificationStep = false;
    }

    onOpened: {
        reset();
        emailField.forceActiveFocus();
    }

    Connections {
        target: CloudAccountController
        function onLoginSucceeded() {
            root.close();
        }
        function onTwoFactorRequired() {
            root.verificationStep = true;
            errorLabel.text = "";
            codeField.forceActiveFocus();
        }
        function onLoginFailed(reason) {
            errorLabel.text = reason.length > 0 ? reason : i18n("Login failed.");
        }
    }

    ColumnLayout {
        width: Kirigami.Units.gridUnit * 22
        spacing: Kirigami.Units.smallSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            text: i18n("This connects using an unofficial, reverse-engineered Bambu Cloud API that Bambu Lab does not document or support. It may stop working without notice.")
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true
            visible: !root.verificationStep

            QQC2.TextField {
                id: emailField
                Kirigami.FormData.label: i18n("Email:")
            }
            QQC2.TextField {
                id: passwordField
                Kirigami.FormData.label: i18n("Password:")
                echoMode: TextInput.Password
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: root.verificationStep

            PlasmaComponents3.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: i18n("A verification code was emailed to you. Enter it below.")
            }

            Kirigami.FormLayout {
                Layout.fillWidth: true

                QQC2.TextField {
                    id: codeField
                    Kirigami.FormData.label: i18n("Verification code:")
                }
            }
        }

        PlasmaComponents3.Label {
            id: errorLabel
            Layout.fillWidth: true
            visible: text.length > 0
            color: Kirigami.Theme.negativeTextColor
            wrapMode: Text.WordWrap
        }

        PlasmaComponents3.Button {
            Layout.alignment: Qt.AlignRight
            text: root.verificationStep ? i18n("Submit Code") : i18n("Log In")
            onClicked: {
                if (root.verificationStep) {
                    if (codeField.text.trim().length === 0) {
                        errorLabel.text = i18n("Enter the verification code.");
                        return;
                    }
                    CloudAccountController.submitVerificationCode(codeField.text.trim());
                } else {
                    if (emailField.text.trim().length === 0 || passwordField.text.length === 0) {
                        errorLabel.text = i18n("Email and password are required.");
                        return;
                    }
                    CloudAccountController.login(emailField.text.trim(), passwordField.text);
                }
            }
        }
    }
}
