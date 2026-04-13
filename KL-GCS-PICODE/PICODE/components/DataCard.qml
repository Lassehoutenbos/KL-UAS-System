import QtQuick
import QtQuick.Layouts
import PICODE

Rectangle {
    width: 156
    height: 100
    color: Theme.bgSecondary
    border.color: Theme.border
    border.width: 1
    radius: 4

    property string label: "LABEL"
    property string value: "0.0"
    property string unit:  "unit"

    // 2px top accent bar
    Rectangle {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 2; radius: 1
        color: Theme.accentBlue
        opacity: 0.7
    }

    // Top-left corner bracket
    Rectangle { anchors.top: parent.top; anchors.left: parent.left; width: 10; height: 1.5; color: Theme.accentBlue; opacity: 0.5 }
    Rectangle { anchors.top: parent.top; anchors.left: parent.left; width: 1.5; height: 10; color: Theme.accentBlue; opacity: 0.5 }

    // Bottom-right corner bracket
    Rectangle { anchors.bottom: parent.bottom; anchors.right: parent.right; width: 10; height: 1.5; color: Theme.accentBlue; opacity: 0.3 }
    Rectangle { anchors.bottom: parent.bottom; anchors.right: parent.right; width: 1.5; height: 10; color: Theme.accentBlue; opacity: 0.3 }

    // Label — top-left
    Text {
        anchors { top: parent.top; left: parent.left; topMargin: 7; leftMargin: 8 }
        text: label
        color: Theme.textSecondary
        font.pixelSize: Theme.fontUnit
        font.weight: Font.Medium
        font.letterSpacing: 0.5
    }

    // Value — centered
    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 3
        text: value
        color: Theme.textPrimary
        font.pixelSize: Theme.fontValueLarge
        font.weight: Font.Bold
        font.family: "monospace"
    }

    // Unit — bottom-right
    Text {
        anchors { bottom: parent.bottom; right: parent.right; bottomMargin: 7; rightMargin: 8 }
        text: unit
        color: Theme.textDisabled
        font.pixelSize: Theme.fontUnit
        font.letterSpacing: 0.3
    }
}
