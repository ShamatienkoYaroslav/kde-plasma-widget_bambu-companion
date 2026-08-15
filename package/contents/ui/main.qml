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

        AddPrinterDialog {
            id: addPrinterDialog
        }

        CertificateConfirmDialog {
            id: certificateConfirmDialog
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
                        icon.name: "list-add"
                        text: i18n("Add Printer…")
                        display: PlasmaComponents3.ToolButton.IconOnly
                        PlasmaComponents3.ToolTip.text: text
                        PlasmaComponents3.ToolTip.visible: hovered
                        onClicked: addPrinterDialog.open()
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
                    Kirigami.Icon {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                        Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                        source: "printer"
                        opacity: 0.5
                    }
                    PlasmaComponents3.Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: i18n("No printers added yet")
                        opacity: 0.7
                    }
                    PlasmaComponents3.Button {
                        Layout.alignment: Qt.AlignHCenter
                        text: i18n("Add Printer…")
                        onClicked: addPrinterDialog.open()
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
