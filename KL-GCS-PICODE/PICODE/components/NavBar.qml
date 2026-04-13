import QtQuick
import QtQuick.Layouts
import PICODE

Rectangle {
    id: navRoot
    color: Theme.bgSecondary

    // Top separator line
    Rectangle {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 1
        color: Theme.border
    }

    signal tabClicked(string pageKey)
    property string currentPage: "dash"

    property var tabs: [
        { label: "DASH",    key: "dash",    icon: "⬡" },
        { label: "DRONE",   key: "drone",   icon: "✈" },
        { label: "CAMERA",  key: "camera",  icon: "◉" },
        { label: "MAP",     key: "map",     icon: "⊞" },
        { label: "MISSION", key: "mission", icon: "◎" },
        { label: "PARAMS",  key: "params",  icon: "≡" },
        { label: "PAYLOAD", key: "payload", icon: "▼" },
        { label: "PERIPH",  key: "periph",  icon: "⬡" },
        { label: "CASE",    key: "case",    icon: "⚙" }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Repeater {
            model: navRoot.tabs

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true

                property bool isActive: modelData.key === currentPage

                // Hover + active tinted background
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        orientation: Gradient.Vertical
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop {
                            position: 1.0
                            color: isActive
                                   ? Qt.rgba(0.89, 0.82, 0.29, 0.11)
                                   : (hoverArea.containsMouse ? Qt.rgba(1,1,1,0.03) : "transparent")
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                    }
                }

                // Left divider (between tabs)
                Rectangle {
                    anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                    width: 1
                    color: Theme.border
                    visible: index > 0
                }

                // Active bottom accent bar
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                    height: 3
                    color: Theme.accentYellow
                    opacity: isActive ? 1.0 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }

                // Icon + label
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: modelData.icon
                        color: isActive ? Theme.accentYellow : Theme.textDisabled
                        font.pixelSize: 15
                        Layout.alignment: Qt.AlignHCenter
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    Text {
                        text: modelData.label
                        color: isActive ? Theme.accentYellow : Theme.textSecondary
                        font.pixelSize: 10
                        font.weight: isActive ? Font.Bold : Font.Medium
                        font.letterSpacing: 0.6
                        Layout.alignment: Qt.AlignHCenter
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }

                MouseArea {
                    id: hoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: tabClicked(modelData.key)
                }
            }
        }
    }
}
