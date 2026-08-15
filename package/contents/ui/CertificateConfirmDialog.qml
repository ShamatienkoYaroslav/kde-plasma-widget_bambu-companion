import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import io.github.shamatienkoyaroslav.bambucompanion

Kirigami.OverlaySheet {
    id: root

    property string printerId: ""
    property string printerName: ""
    property string fingerprint: ""

    title: i18n("Verify Printer Certificate")

    Connections {
        target: PrinterController
        function onCertificateTrustNeeded(id, name, fp) {
            root.printerId = id;
            root.printerName = name;
            root.fingerprint = fp;
            root.open();
        }
    }

    ColumnLayout {
        width: Kirigami.Units.gridUnit * 22
        spacing: Kirigami.Units.smallSpacing

        PlasmaComponents3.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("This is the first time connecting to \"%1\". Verify that this certificate fingerprint matches what you expect before trusting it — anyone able to intercept your local network traffic could otherwise present a fake certificate.", root.printerName)
        }

        PlasmaComponents3.Label {
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
            font.family: "monospace"
            text: root.fingerprint
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents3.Button {
                text: i18n("Reject")
                onClicked: {
                    PrinterController.confirmCertificateTrust(root.printerId, root.fingerprint, false);
                    root.close();
                }
            }
            PlasmaComponents3.Button {
                text: i18n("Trust")
                onClicked: {
                    PrinterController.confirmCertificateTrust(root.printerId, root.fingerprint, true);
                    root.close();
                }
            }
        }
    }
}
