import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3

Kirigami.OverlaySheet {
    id: root

    property string message: ""
    property var acceptedCallback: null

    title: i18n("Confirm Action")

    ColumnLayout {
        width: Kirigami.Units.gridUnit * 18
        spacing: Kirigami.Units.smallSpacing

        PlasmaComponents3.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: root.message
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents3.Button {
                text: i18n("Cancel")
                onClicked: root.close()
            }
            PlasmaComponents3.Button {
                text: i18n("Confirm")
                onClicked: {
                    root.close();
                    if (root.acceptedCallback) {
                        root.acceptedCallback();
                    }
                }
            }
        }
    }
}
