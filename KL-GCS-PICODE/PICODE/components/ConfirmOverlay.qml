import QtQuick
import QtQuick.Layouts
import PICODE

Rectangle {
    id: overlay
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.6)
    visible: false
    z: 100

    property string title: "Confirm"
    property string body: "Are you sure?"
    property string confirmLabel: "CONFIRM"
    property bool destructive: false

    signal confirmed()
    signal cancelled()

    function open() { visible = true }
    function close() { visible = false }

    MouseArea {
        anchors.fill: parent
        onClicked: overlay.close()
    }

    Rectangle {
        anchors.centerIn: parent
        width: 440
        height: 220
        color: Theme.bgSecondary
        border.color: Theme.border
        border.width: 1
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                text: overlay.title
                color: Theme.textPrimary
                font.pixelSize: 16
                font.weight: Font.SemiBold
            }

            Text {
                text: overlay.body
                color: Theme.textSecondary
                font.pixelSize: Theme.fontButton
                wrapMode: Text.Wrap
                Layout.fillHeight: true
            }

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                BigButton {
                    label: "CANCEL"
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 64
                    onClicked: overlay.close()
                }

                BigButton {
                    label: overlay.confirmLabel
                    active: true
                    danger: overlay.destructive
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 64
                    onClicked: {
                        overlay.confirmed()
                        overlay.close()
                    }
                }
            }
        }
    }
}
