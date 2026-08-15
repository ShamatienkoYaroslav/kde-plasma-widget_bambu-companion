import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3

Kirigami.OverlaySheet {
    id: root

    property string printerId: ""

    signal submitted(string printerId, var objectIds)

    title: i18n("Skip Objects")

    onOpened: {
        idsField.text = "";
        idsField.forceActiveFocus();
    }

    ColumnLayout {
        width: Kirigami.Units.gridUnit * 18
        spacing: Kirigami.Units.smallSpacing

        PlasmaComponents3.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Enter the object IDs to skip, separated by commas.")
        }

        QQC2.TextField {
            id: idsField
            Layout.fillWidth: true
            placeholderText: "1, 2, 3"
        }

        PlasmaComponents3.Button {
            Layout.alignment: Qt.AlignRight
            text: i18n("Skip")
            onClicked: {
                const ids = idsField.text.split(",").map(s => parseInt(s.trim(), 10)).filter(n => !isNaN(n));
                if (ids.length > 0) {
                    root.submitted(root.printerId, ids);
                    root.close();
                }
            }
        }
    }
}
