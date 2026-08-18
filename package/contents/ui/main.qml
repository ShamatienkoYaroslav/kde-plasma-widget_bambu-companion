import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami
import io.github.shamatienkoyaroslav.bambucompanion

PlasmoidItem {
    id: root

    property string selectedPrinterId: ""

    compactRepresentation: MouseArea {
        onClicked: root.expanded = !root.expanded

        Kirigami.Icon {
            anchors.fill: parent
            source: Plasmoid.icon
        }
    }

    fullRepresentation: ColumnLayout {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        Layout.preferredHeight: Kirigami.Units.gridUnit * 18

        CertificateConfirmDialog {
            id: certificateConfirmDialog
        }

        CloudLoginDialog {
            id: cloudLoginDialog
        }

        Component {
            id: listComponent

            ColumnLayout {
                RowLayout {
                    Layout.fillWidth: true

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 2
                        text: i18n("Bambu Companion")
                    }
                    PlasmaComponents3.ToolButton {
                        icon.name: CloudAccountController.loggedIn ? "im-user-away" : "im-user"
                        text: CloudAccountController.loggedIn ? i18n("Log out of Bambu Cloud") : i18n("Log into Bambu Cloud…")
                        display: PlasmaComponents3.ToolButton.IconOnly
                        PlasmaComponents3.ToolTip.text: text
                        PlasmaComponents3.ToolTip.visible: hovered
                        onClicked: CloudAccountController.loggedIn ? CloudAccountController.logout() : cloudLoginDialog.open()
                    }
                }

                ListView {
                    id: printerListView

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    visible: count > 0

                    model: PrinterListModel

                    delegate: PrinterListItem {
                        width: printerListView.width
                        onClicked: root.selectedPrinterId = model.printerId
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: printerListView.count === 0

                    Item {
                        Layout.fillHeight: true
                    }
                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: Kirigami.Units.iconSizes.huge * 3
                        Layout.preferredHeight: Kirigami.Units.iconSizes.huge * 3
                        fillMode: Image.PreserveAspectFit
                        source: "../images/bambulab-p1s.png"
                        opacity: 0.6
                    }
                    PlasmaComponents3.Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: CloudAccountController.loggedIn
                            ? i18n("No printers found on your Bambu account")
                            : i18n("Log into Bambu Cloud to see your printers")
                        opacity: 0.7
                    }
                    PlasmaComponents3.Button {
                        Layout.alignment: Qt.AlignHCenter
                        visible: !CloudAccountController.loggedIn
                        text: i18n("Log into Bambu Cloud…")
                        onClicked: cloudLoginDialog.open()
                    }
                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }

        Component {
            id: detailComponent

            PrinterDetailView {
                printerId: root.selectedPrinterId
                onBackRequested: root.selectedPrinterId = ""
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: root.selectedPrinterId.length > 0 ? detailComponent : listComponent
        }
    }
}
