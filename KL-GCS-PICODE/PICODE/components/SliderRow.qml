import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PICODE

Rectangle {
    id: sliderRowRoot
    height: 48
    color: "transparent"

    property string label:    "SLIDE"
    property int    value:    50
    property string unit:     "%"
    property bool   readOnly: false
    property int    labelWidth: 120
    property int    valueWidth: 52

    // Use sliderMoved (not valueChanged) to avoid clashing with the
    // auto-generated property-change notification for `value`.
    signal sliderMoved(int v)

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Text {
            text: sliderRowRoot.label
            color: Theme.textSecondary
            font.pixelSize: Theme.fontUnit
            font.weight: Font.Medium
            font.letterSpacing: 0.4
            Layout.preferredWidth: sliderRowRoot.labelWidth
        }

        Slider {
            id: sl
            from: 0; to: 100; stepSize: 1
            value: sliderRowRoot.value
            enabled: !sliderRowRoot.readOnly
            Layout.fillWidth: true

            // ── Custom track ────────────────────────────────────────────────
            background: Item {
                x: sl.leftPadding
                y: sl.topPadding + sl.availableHeight / 2 - height / 2
                implicitWidth: 200; implicitHeight: 6
                width: sl.availableWidth; height: 6

                // Track background
                Rectangle {
                    width: parent.width; height: parent.height; radius: 3
                    color: Theme.bgElevated
                    border.color: Theme.border; border.width: 1
                }
                // Filled portion
                Rectangle {
                    width: sl.visualPosition * parent.width
                    height: parent.height; radius: 3
                    color: sl.enabled ? Theme.accentBlue : Theme.textDisabled
                    opacity: 0.85
                }
            }

            // ── Custom handle ────────────────────────────────────────────────
            handle: Rectangle {
                x: sl.leftPadding + sl.visualPosition * (sl.availableWidth - width)
                y: sl.topPadding + sl.availableHeight / 2 - height / 2
                width: 18; height: 18; radius: 9
                color: sl.enabled ? Theme.accentYellow : Theme.textDisabled
                border.color: Theme.bgPrimary; border.width: 2

                // Inner dot
                Rectangle {
                    anchors.centerIn: parent
                    width: 6; height: 6; radius: 3
                    color: Theme.bgPrimary
                    opacity: 0.5
                }

                scale: sl.pressed ? 0.84 : 1.0
                Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }
            }

            onMoved: sliderRowRoot.sliderMoved(Math.round(value))
        }

        Text {
            text: sliderRowRoot.value + sliderRowRoot.unit
            color: Theme.textPrimary
            font.pixelSize: Theme.fontUnit
            font.family: "monospace"
            Layout.preferredWidth: sliderRowRoot.valueWidth
            horizontalAlignment: Text.AlignRight
        }
    }
}
