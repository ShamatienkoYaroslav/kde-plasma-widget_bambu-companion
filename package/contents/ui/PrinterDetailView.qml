import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami
import io.github.shamatienkoyaroslav.bambucompanion

ColumnLayout {
    id: root

    property string printerId: ""
    property var details: ({})
    property string errorMessage: ""

    signal backRequested

    function refresh() {
        if (root.printerId.length > 0) {
            root.details = PrinterListModel.printerDetails(root.printerId);
        }
    }

    onPrinterIdChanged: refresh()
    Component.onCompleted: refresh()

    Connections {
        target: PrinterListModel
        function onDataChanged() {
            root.refresh();
        }
    }

    Connections {
        target: PrinterController
        function onCommandFailed(printerId, reason) {
            if (printerId === root.printerId) {
                root.errorMessage = reason.length > 0 ? reason : i18n("Command failed.");
            }
        }
    }

    ConfirmActionDialog {
        id: confirmStopDialog
        message: i18n("Stop the current print? This cannot be undone.")
        acceptedCallback: () => PrinterController.stop(root.printerId)
    }

    SkipObjectsDialog {
        id: skipObjectsDialog
        printerId: root.printerId
        onSubmitted: (printerId, objectIds) => PrinterController.skipObjects(printerId, objectIds)
    }

    RowLayout {
        Layout.fillWidth: true

        PlasmaComponents3.ToolButton {
            icon.name: "go-previous"
            text: i18n("Back")
            onClicked: root.backRequested()
        }
        PlasmaComponents3.Label {
            Layout.fillWidth: true
            text: root.details.name || ""
            font.bold: true
            elide: Text.ElideRight
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: root.errorMessage.length > 0
        text: root.errorMessage

        Timer {
            running: root.errorMessage.length > 0
            interval: 6000
            onTriggered: root.errorMessage = ""
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing

        PlasmaComponents3.Button {
            text: i18n("Pause")
            visible: root.details.status === "Printing"
            onClicked: PrinterController.pause(root.printerId)
        }
        PlasmaComponents3.Button {
            text: i18n("Resume")
            visible: root.details.status === "Paused"
            onClicked: PrinterController.resume(root.printerId)
        }
        PlasmaComponents3.Button {
            text: i18n("Stop")
            visible: root.details.status === "Printing" || root.details.status === "Paused"
            onClicked: confirmStopDialog.open()
        }
        PlasmaComponents3.Button {
            text: i18n("Skip Objects…")
            visible: root.details.status === "Printing"
            onClicked: skipObjectsDialog.open()
        }
    }

    Kirigami.FormLayout {
        Layout.fillWidth: true

        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Status:")
            text: root.details.status || ""
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Progress:")
            visible: (root.details.progress ?? -1) >= 0
            text: (root.details.progress ?? 0) + "%"
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Layer:")
            visible: (root.details.currentLayer ?? -1) >= 0
            text: root.details.currentLayer + " / " + root.details.totalLayers
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Remaining time:")
            visible: (root.details.remainingMinutes ?? -1) >= 0
            text: i18np("%1 minute", "%1 minutes", root.details.remainingMinutes ?? 0)
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Nozzle:")
            visible: root.details.nozzleTemp !== undefined && root.details.nozzleTemp !== null
            text: (root.details.nozzleTemp ?? 0) + " / " + (root.details.nozzleTargetTemp ?? 0) + " °C"
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Bed:")
            visible: root.details.bedTemp !== undefined && root.details.bedTemp !== null
            text: (root.details.bedTemp ?? 0) + " / " + (root.details.bedTargetTemp ?? 0) + " °C"
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Chamber:")
            visible: root.details.chamberTemp !== undefined && root.details.chamberTemp !== null
            text: (root.details.chamberTemp ?? 0) + " °C"
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Fans:")
            visible: (root.details.coolingFanPercent ?? -1) >= 0
            text: i18n("%1% part / %2% aux / %3% chamber", root.details.coolingFanPercent ?? 0, root.details.auxFanPercent ?? 0, root.details.chamberFanPercent ?? 0)
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Speed profile:")
            visible: (root.details.speedProfile || "").length > 0
            text: root.details.speedProfile || ""
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("WiFi signal:")
            visible: (root.details.wifiSignalDbm ?? 0) !== 0
            text: (root.details.wifiSignalDbm ?? 0) + " dBm"
        }
        PlasmaComponents3.Label {
            Kirigami.FormData.label: i18n("Address:")
            text: root.details.host || ""
        }
    }

    Item {
        Layout.fillHeight: true
    }
}
