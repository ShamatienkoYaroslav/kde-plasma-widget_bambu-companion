import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami
import io.github.shamatienkoyaroslav.bambucompanion

PlasmoidItem {
    id: root

    PrinterListModel {
        id: printerListModel
    }

    compactRepresentation: MouseArea {
        onClicked: root.expanded = !root.expanded

        Kirigami.Icon {
            anchors.fill: parent
            source: Plasmoid.icon
        }
    }

    fullRepresentation: ColumnLayout {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 18
        Layout.preferredHeight: Kirigami.Units.gridUnit * 14

        ListView {
            id: printerListView

            Layout.fillWidth: true
            Layout.fillHeight: true

            model: printerListModel

            delegate: PlasmaComponents3.ItemDelegate {
                width: printerListView.width
                text: model.name + " — " + model.status
            }
        }
    }
}
