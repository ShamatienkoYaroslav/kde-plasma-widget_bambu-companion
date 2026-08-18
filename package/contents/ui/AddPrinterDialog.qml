import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import io.github.shamatienkoyaroslav.bambucompanion

Kirigami.OverlaySheet {
    id: root

    property int mode: 0 // 0 = manual LAN entry, 1 = pick from Bambu Cloud account
    property var cloudDevices: []

    // CloudLoginDialog is intentionally NOT declared here: Kirigami.OverlaySheet
    // treats default children as scrollable sheet content (its
    // `flickableContentData` default property), which badly mis-sizes a
    // nested Popup-derived type like another OverlaySheet. It lives as a
    // sibling in main.qml instead; this signal asks it to open.
    signal cloudLoginRequested

    title: i18n("Add Printer")

    function reset() {
        nameField.text = "";
        hostField.text = "";
        serialField.text = "";
        accessCodeField.text = "";
        portField.text = "8883";
        errorLabel.text = "";
        root.mode = 0;
        root.cloudDevices = [];
    }

    onOpened: {
        reset();
        nameField.forceActiveFocus();
    }

    Connections {
        target: CloudAccountController
        function onLoginSucceeded() {
            CloudAccountController.fetchDevices();
        }
        function onDevicesReady(devices) {
            root.cloudDevices = devices;
        }
        function onFetchFailed(reason) {
            errorLabel.text = reason;
        }
    }

    ColumnLayout {
        width: Kirigami.Units.gridUnit * 20
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true

            PlasmaComponents3.Button {
                Layout.fillWidth: true
                text: i18n("Add Manually (LAN)")
                checkable: true
                checked: root.mode === 0
                onClicked: root.mode = 0
            }
            PlasmaComponents3.Button {
                Layout.fillWidth: true
                text: i18n("From My Bambu Account")
                checkable: true
                checked: root.mode === 1
                onClicked: {
                    root.mode = 1;
                    if (CloudAccountController.loggedIn) {
                        CloudAccountController.fetchDevices();
                    }
                }
            }
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true
            visible: root.mode === 0

            QQC2.TextField {
                id: nameField
                Kirigami.FormData.label: i18n("Name:")
                placeholderText: i18n("My Printer")
            }
            QQC2.TextField {
                id: hostField
                Kirigami.FormData.label: i18n("Host / IP:")
                placeholderText: "192.168.1.50"
            }
            QQC2.TextField {
                id: serialField
                Kirigami.FormData.label: i18n("Serial number:")
            }
            QQC2.TextField {
                id: accessCodeField
                Kirigami.FormData.label: i18n("LAN access code:")
                echoMode: TextInput.Password
            }
            QQC2.TextField {
                id: portField
                Kirigami.FormData.label: i18n("MQTT port:")
                text: "8883"
                validator: IntValidator {
                    bottom: 1
                    top: 65535
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: root.mode === 1

            PlasmaComponents3.Button {
                Layout.alignment: Qt.AlignHCenter
                visible: !CloudAccountController.loggedIn
                text: i18n("Log into Bambu Cloud…")
                onClicked: root.cloudLoginRequested()
            }

            PlasmaComponents3.Label {
                Layout.alignment: Qt.AlignHCenter
                visible: CloudAccountController.loggedIn && root.cloudDevices.length === 0
                text: i18n("No printers found on this account.")
                opacity: 0.7
            }

            ListView {
                id: cloudDeviceListView

                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 10
                clip: true
                visible: CloudAccountController.loggedIn && root.cloudDevices.length > 0

                model: root.cloudDevices

                delegate: PlasmaComponents3.ItemDelegate {
                    width: cloudDeviceListView.width
                    text: modelData.online ? modelData.name : i18n("%1 (offline)", modelData.name)
                    onClicked: {
                        CloudAccountController.addCloudPrinter(modelData.devId, modelData.name);
                        root.close();
                    }
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
            visible: root.mode === 0
            text: i18n("Add")
            onClicked: {
                if (nameField.text.trim().length === 0
                        || hostField.text.trim().length === 0
                        || serialField.text.trim().length === 0
                        || accessCodeField.text.length === 0) {
                    errorLabel.text = i18n("Name, host, serial number, and access code are all required.");
                    return;
                }

                PrinterController.addLanPrinter(nameField.text.trim(), hostField.text.trim(), serialField.text.trim(), accessCodeField.text, parseInt(portField.text, 10) || 8883);
                root.close();
            }
        }
    }
}
