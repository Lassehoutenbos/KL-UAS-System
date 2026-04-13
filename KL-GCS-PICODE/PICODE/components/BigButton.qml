import QtQuick
import PICODE

Rectangle {
    id: btn
    width: 220
    height: 72
    radius: 6
    border.width: 1

    property string label:   "BUTTON"
    property bool   active:  false
    property bool   danger:  false

    border.color: {
        if (!enabled)         return Theme.bgElevated
        if (active && danger) return Theme.statusCrit
        if (active)           return Theme.accentYellow
        return hoverArea.containsMouse ? Qt.lighter(Theme.border, 1.6) : Theme.border
    }

    color: {
        if (!enabled) return Theme.bgSecondary
        if (active)   return danger ? Qt.rgba(1,0.36,0.36,0.18) : Qt.rgba(0.89,0.82,0.29,0.14)
        return hoverArea.containsMouse ? Theme.bgElevated : Qt.rgba(0.1,0.12,0.16,1)
    }

    // Subtle top highlight on active
    Rectangle {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 1; radius: 1
        color: active ? (danger ? Theme.statusCrit : Theme.accentYellow) : "transparent"
        opacity: 0.6
    }

    Behavior on color       { ColorAnimation  { duration: 100 } }
    Behavior on border.color{ ColorAnimation  { duration: 100 } }
    Behavior on scale       { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }

    scale: hoverArea.pressed ? 0.96 : 1.0

    Text {
        anchors.centerIn: parent
        text: label
        color: {
            if (!enabled)         return Theme.textDisabled
            if (active && danger) return Theme.statusCrit
            if (active)           return Theme.accentYellow
            return Theme.textPrimary
        }
        font.pixelSize: Theme.fontButton
        font.weight: Font.SemiBold
        font.letterSpacing: 0.4
        Behavior on color { ColorAnimation { duration: 100 } }
    }

    signal clicked()

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        enabled: btn.enabled
        onClicked: btn.clicked()
    }
}
