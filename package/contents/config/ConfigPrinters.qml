import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import io.github.shamatienkoyaroslav.bambucompanion

Kirigami.FormLayout {
    id: root

    ListView {
        Kirigami.FormData.label: i18n("Configured printers:")
        Layout.fillWidth: true
        Layout.preferredHeight: Kirigami.Units.gridUnit * 10
        clip: true

        model: PrinterListModel

        delegate: RowLayout {
            width: ListView.view.width

            QQC2.Label {
                Layout.fillWidth: true
                text: model.name + " (" + model.status + ")"
                elide: Text.ElideRight
            }
            QQC2.ToolButton {
                icon.name: "edit-delete"
                text: i18n("Remove")
                display: QQC2.ToolButton.IconOnly
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
                onClicked: PrinterController.removePrinter(model.printerId)
            }
        }
    }

    QQC2.Label {
        text: i18n("Use the widget's popup to add a new printer. No access codes are ever shown here.")
        opacity: 0.7
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
