import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

PlasmaComponents3.ItemDelegate {
    id: root

    contentItem: ColumnLayout {
        spacing: 0

        PlasmaComponents3.Label {
            Layout.fillWidth: true
            text: model.name
            elide: Text.ElideRight
            font.bold: true
        }
        PlasmaComponents3.Label {
            Layout.fillWidth: true
            text: model.progress >= 0 ? (model.status + " — " + model.progress + "%") : model.status
            elide: Text.ElideRight
            opacity: 0.7
        }
    }
}
