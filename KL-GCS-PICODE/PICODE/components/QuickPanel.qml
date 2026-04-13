import QtQuick
import QtQuick.Layouts
import PICODE

Item {
    id: panel

    function toggle() {
        if (visible) {
            hideAnim.start()
        } else {
            visible = true
            showAnim.start()
        }
    }

    NumberAnimation {
        id: showAnim
        target: panelRect
        property: "y"
        to: 0
        duration: 150
        easing.type: Easing.OutCubic
    }

    NumberAnimation {
        id: hideAnim
        target: panelRect
        property: "y"
        to: -panelRect.height
        duration: 150
        easing.type: Easing.OutCubic
        onStopped: panel.visible = false
    }

    // Scrim (click to dismiss)
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.45)

        MouseArea {
            anchors.fill: parent
            onClicked: panel.toggle()
        }
    }

    // Slide-down panel
    Rectangle {
        id: panelRect
        width: parent.width
        height: 280
        y: -height
        color: Theme.bgSecondary
        border.color: Theme.border
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            RowLayout {
                spacing: 8
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                Text {
                    text: "QUICK PANEL"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontPageTitle
                    font.weight: Font.SemiBold
                    Layout.fillWidth: true
                }

                BigButton {
                    label: "✕"
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    onClicked: panel.toggle()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            RowLayout {
                spacing: 8
                Layout.fillWidth: true
                Layout.preferredHeight: 80

                BigButton {
                    label: "MAP: ONLINE"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                BigButton {
                    label: "ALS: ON"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                Text {
                    text: "PI CPU " + GCSState.tempCpuPi.toFixed(0) + "°C  │  MEM " + GCSState.memPercent + "%  │  DISK " + GCSState.diskPercent + "%"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontUnit
                    Layout.fillWidth: true
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
