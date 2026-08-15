import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import io.github.shamatienkoyaroslav.bambucompanion

Kirigami.OverlaySheet {
    id: root

    title: i18n("Add Printer")

    function reset() {
        nameField.text = "";
        hostField.text = "";
        serialField.text = "";
        accessCodeField.text = "";
        portField.text = "8883";
        errorLabel.text = "";
    }

    onOpened: {
        reset();
        nameField.forceActiveFocus();
    }

    ColumnLayout {
        width: Kirigami.Units.gridUnit * 20

        Kirigami.FormLayout {
            Layout.fillWidth: true

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

        PlasmaComponents3.Label {
            id: errorLabel
            Layout.fillWidth: true
            visible: text.length > 0
            color: Kirigami.Theme.negativeTextColor
            wrapMode: Text.WordWrap
        }

        PlasmaComponents3.Button {
            Layout.alignment: Qt.AlignRight
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
