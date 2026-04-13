import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PICODE

Rectangle {
    color: Theme.bgPrimary

    // Compute current TFT mode from GCSState (mirrors picolink updateTftMode logic)
    readonly property int tftMode: {
        if (GCSState.batteryPercent < 15)          return 4
        if (GCSState.anyWarningActive)             return 2
        if (!GCSState.keyUnlocked)                 return 3
        if (GCSState.peripherals.length > 0)       return 5
        return 1
    }

    property bool screensLinked: true

    // Stored ratios for proportional master-slider scaling
    property var brightnessRatios: [1.0, 1.0, 1.0, 1.0, 1.0]

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 0

            // ── BRIGHTNESS ────────────────────────────────────────────────────
            Item {
                Layout.fillWidth: true; height: 32
                Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentYellow }
                Text {
                    anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                    text: "BRIGHTNESS"
                    color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 12
                spacing: 0

                // MASTER
                RowLayout {
                    Layout.fillWidth: true
                    height: 48
                    spacing: 8

                    Text {
                        text: "MASTER"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontUnit
                        font.weight: Font.Medium
                        Layout.preferredWidth: 120
                    }

                    Slider {
                        id: masterSlider
                        from: 0; to: 100; stepSize: 1
                        value: GCSState.brightnessScreenL
                        Layout.fillWidth: true

                        onPressedChanged: {
                            if (pressed) {
                                var cur = value || 1
                                brightnessRatios = [
                                    GCSState.brightnessScreenL / cur,
                                    GCSState.brightnessScreenR / cur,
                                    GCSState.brightnessLed     / cur,
                                    GCSState.brightnessTft     / cur,
                                    GCSState.brightnessBtnLeds / cur
                                ]
                            }
                        }
                        onMoved: {
                            var v = Math.round(value)
                            GCSState.brightnessScreenL = Math.min(100, Math.round(brightnessRatios[0] * v))
                            GCSState.brightnessScreenR = screensLinked
                                ? GCSState.brightnessScreenL
                                : Math.min(100, Math.round(brightnessRatios[1] * v))
                            GCSState.brightnessLed     = Math.min(100, Math.round(brightnessRatios[2] * v))
                            GCSState.brightnessTft     = Math.min(100, Math.round(brightnessRatios[3] * v))
                            GCSState.brightnessBtnLeds = Math.min(100, Math.round(brightnessRatios[4] * v))
                        }
                    }

                    Text {
                        text: Math.round(masterSlider.value) + "%"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontUnit
                        Layout.preferredWidth: 60
                    }
                }

                // SCREENS + link toggle
                RowLayout {
                    Layout.fillWidth: true
                    height: 48
                    spacing: 8

                    SliderRow {
                        label: screensLinked ? "SCREENS" : "SCREEN L"
                        value: GCSState.brightnessScreenL
                        Layout.fillWidth: true
                        onSliderMoved: function(v) {
                            GCSState.brightnessScreenL = v
                            if (screensLinked) GCSState.brightnessScreenR = v
                        }
                    }

                    Rectangle {
                        width: 110; height: 40; radius: 4
                        color: screensLinked ? Theme.accentYellow : Theme.bgElevated
                        border.color: screensLinked ? Theme.accentYellow : Theme.border
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: screensLinked ? "LINK ■" : "UNLINK □"
                            color: screensLinked ? Theme.bgPrimary : Theme.textSecondary
                            font.pixelSize: Theme.fontSectionLabel
                            font.weight: Font.SemiBold
                        }
                        MouseArea { anchors.fill: parent; onClicked: screensLinked = !screensLinked }
                    }
                }

                // SCREEN R — only when unlinked
                SliderRow {
                    visible: !screensLinked
                    label: "SCREEN R"
                    value: GCSState.brightnessScreenR
                    Layout.fillWidth: true
                    onSliderMoved: function(v) { GCSState.brightnessScreenR = v }
                }

                SliderRow {
                    label: "LED STRIP"
                    value: GCSState.brightnessLed
                    Layout.fillWidth: true
                    onSliderMoved: function(v) { GCSState.brightnessLed = v }
                }
                SliderRow {
                    label: "TFT BL"
                    value: GCSState.brightnessTft
                    Layout.fillWidth: true
                    onSliderMoved: function(v) { GCSState.brightnessTft = v }
                }
                SliderRow {
                    label: "BTN LEDS"
                    value: GCSState.brightnessBtnLeds
                    Layout.fillWidth: true
                    onSliderMoved: function(v) { GCSState.brightnessBtnLeds = v }
                }

                // ALS auto-brightness toggle
                RowLayout {
                    Layout.fillWidth: true
                    height: 48
                    spacing: 8

                    Text {
                        text: "AMBIENT LIGHT AUTO-BRIGHTNESS"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSectionLabel
                        font.weight: Font.Medium
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        width: 120; height: 40; radius: 4
                        color: GCSState.alsAutoEnabled ? Theme.accentYellow : Theme.bgElevated
                        border.color: GCSState.alsAutoEnabled ? Theme.accentYellow : Theme.border
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: GCSState.alsAutoEnabled ? "ENABLED ▶" : "DISABLED"
                            color: GCSState.alsAutoEnabled ? Theme.bgPrimary : Theme.textSecondary
                            font.pixelSize: Theme.fontSectionLabel
                            font.weight: Font.SemiBold
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: GCSState.alsAutoEnabled = !GCSState.alsAutoEnabled
                        }
                    }
                }

                Text {
                    visible: GCSState.alsAutoEnabled
                    text: "ALS sensor adjusts all outputs automatically based on lux reading."
                    color: Theme.textDisabled
                    font.pixelSize: Theme.fontSectionLabel
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.bottomMargin: 4
                }
            }

            // ── TEMPERATURES ─────────────────────────────────────────────────
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: 4 }
            Item {
                Layout.fillWidth: true; height: 32
                Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 3; color: Theme.accentBlue }
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter
                    text: "TEMPERATURES"
                    color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            GridLayout {
                Layout.fillWidth: true; Layout.margins: 12
                columns: 2; rowSpacing: 6; columnSpacing: 8

                Repeater {
                    model: [
                        { lbl: "PI CPU",   temp: GCSState.tempCpuPi,  valid: true },
                        { lbl: "CASE CH2", temp: GCSState.tempCaseA,  valid: !isNaN(GCSState.tempCaseA) && GCSState.tempCaseA > -274 },
                        { lbl: "CASE CH3", temp: GCSState.tempCaseB,  valid: !isNaN(GCSState.tempCaseB) && GCSState.tempCaseB > -274 },
                        { lbl: "CASE CH4", temp: GCSState.tempCaseC,  valid: !isNaN(GCSState.tempCaseC) && GCSState.tempCaseC > -274 },
                        { lbl: "CASE CH5", temp: GCSState.tempCaseD,  valid: !isNaN(GCSState.tempCaseD) && GCSState.tempCaseD > -274 }
                    ]
                    Rectangle {
                        Layout.fillWidth: true; height: 36; radius: 3
                        property int lvl: modelData.valid
                                         ? (modelData.temp < 60 ? 0 : (modelData.temp < 80 ? 1 : 2))
                                         : -1
                        color: lvl === 2 ? Qt.rgba(1,0.36,0.36,0.08)
                             : lvl === 1 ? Qt.rgba(1,0.7,0.28,0.06) : Theme.bgSecondary
                        border.color: lvl === 2 ? Theme.statusCrit
                                    : lvl === 1 ? Theme.statusWarn : Theme.border
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 300 } }
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                            StatusDot { level: parent.parent.lvl }
                            Text { text: modelData.lbl; color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.Medium; Layout.preferredWidth: 72; font.letterSpacing: 0.3 }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: modelData.valid ? modelData.temp.toFixed(0) + "°C" : "—"
                                color: Theme.textPrimary; font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                            }
                        }
                    }
                }

                // Searchlight temp (from RS-485, only if online)
                Rectangle {
                    id: slTempBadge
                    Layout.fillWidth: true; height: 36; radius: 3
                    property var slDev: {
                        for (var i = 0; i < GCSState.peripherals.length; i++) {
                            var d = GCSState.peripherals[i]
                            if (d.name === "SEARCHLIGHT" && d.online) return d
                        }
                        return null
                    }
                    visible: slDev !== null
                    property int lvl: slDev ? (slDev.temp < 60 ? 0 : (slDev.temp < 80 ? 1 : 2)) : -1
                    color: lvl === 2 ? Qt.rgba(1,0.36,0.36,0.08) : lvl === 1 ? Qt.rgba(1,0.7,0.28,0.06) : Theme.bgSecondary
                    border.color: lvl === 2 ? Theme.statusCrit : lvl === 1 ? Theme.statusWarn : Theme.border
                    border.width: 1
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                        StatusDot { level: slTempBadge.lvl }
                        Text { text: "SEARCHLIGHT"; color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.Medium; Layout.preferredWidth: 72; font.letterSpacing: 0.3 }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: slTempBadge.slDev ? slTempBadge.slDev.temp + "°C" : "—"
                            color: Theme.textPrimary; font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                        }
                    }
                }
            }

            // ── TFT SCREEN MODE ──────────────────────────────────────────────
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: 4 }
            Item {
                Layout.fillWidth: true; height: 32
                Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 3; color: Theme.accentBlue; opacity: 0.5 }
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter
                    text: "TFT SCREEN MODE  —  AUTOMATIC"
                    color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 12
                Layout.bottomMargin: 16
                spacing: 6

                Repeater {
                    model: [
                        { mode: 0, label: "AUTO"   },
                        { mode: 1, label: "MAIN"   },
                        { mode: 2, label: "WARN"   },
                        { mode: 3, label: "LOCK"   },
                        { mode: 4, label: "BAT"    },
                        { mode: 5, label: "PERIPH" }
                    ]
                    Rectangle {
                        Layout.fillWidth: true
                        height: 56
                        radius: 4
                        color: Theme.bgElevated
                        border.color: (modelData.mode === tftMode || modelData.mode === 0)
                                      ? Theme.accentYellow : Theme.border
                        border.width:  (modelData.mode === tftMode || modelData.mode === 0) ? 2 : 1
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: (modelData.mode === tftMode || modelData.mode === 0)
                                   ? Theme.accentYellow : Theme.textSecondary
                            font.pixelSize: Theme.fontButton
                            font.weight: Font.SemiBold
                        }
                    }
                }
            }
        }
    }
}
