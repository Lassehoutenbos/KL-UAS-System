import QtQuick
import PICODE

Item {
    width: 10
    height: 10

    property int level: -1

    readonly property color dotColor: {
        switch (level) {
            case 0:  return Theme.statusOk
            case 1:  return Theme.statusWarn
            case 2:  return Theme.statusCrit
            default: return Theme.textDisabled
        }
    }

    // Outer glow ring (bleeds slightly but stays subtle)
    Rectangle {
        anchors.centerIn: parent
        width: 18; height: 18; radius: 9
        color: "transparent"
        border.color: dotColor
        border.width: 1
        opacity: level >= 0 ? 0.28 : 0
    }

    // Inner dot
    Rectangle {
        anchors.centerIn: parent
        width: 8; height: 8; radius: 4
        color: dotColor
    }

    // Blink for critical
    SequentialAnimation on opacity {
        running: level === 2
        loops:   Animation.Infinite
        NumberAnimation { to: 0.3; duration: 700; easing.type: Easing.InOutSine }
        NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
        onStopped: parent.opacity = 1.0
    }
}
