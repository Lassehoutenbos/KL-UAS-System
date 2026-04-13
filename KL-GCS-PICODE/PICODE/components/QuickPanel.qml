import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PICODE

Item {
    id: panel
    property string quickLayoutMode: "PILOT"

    function toggle() {
        if (visible) {
            hideAnim.start()
        } else {
            visible = true
            showAnim.start()
        }
    }

    // Brightness ratios for proportional master-slider scaling
    property var brightnessRatios: [1.0, 1.0, 1.0, 1.0, 1.0]

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
        height: 468
        y: -height
        color: Theme.bgSecondary
        border.color: Theme.border
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            // ── HEADER ──────────────────────────────────────────────────
            RowLayout {
                spacing: 8
                Layout.fillWidth: true
                Layout.preferredHeight: 36

                Text {
                    text: "QUICK PANEL"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontPageTitle
                    font.weight: Font.SemiBold
                    Layout.fillWidth: true
                }

                BigButton {
                    label: "✕"
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    onClicked: panel.toggle()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 188
                radius: 4
                color: Theme.bgElevated
                border.color: Theme.border
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Text {
                        text: "LIGHTING"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSectionLabel
                        font.weight: Font.SemiBold
                        font.letterSpacing: 0.6
                    }

                    // ── QUICK ACTIONS ───────────────────────────────────────
                    RowLayout {
                        spacing: 8
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52

                        // Worklight toggle
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 4
                            color: GCSState.worklightOn ? Qt.rgba(Theme.accentYellow.r, Theme.accentYellow.g, Theme.accentYellow.b, 0.10) : Theme.bgSecondary
                            border.color: GCSState.worklightOn ? Theme.accentYellow : Theme.border
                            border.width: GCSState.worklightOn ? 2 : 1

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2
                                Text {
                                    text: "WORKLIGHT"
                                    color: GCSState.worklightOn ? Theme.accentYellow : Theme.textSecondary
                                    font.pixelSize: 10
                                    font.weight: Font.SemiBold
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Text {
                                    text: GCSState.worklightOn ? "ON" : "OFF"
                                    color: GCSState.worklightOn ? Theme.accentYellow : Theme.textDisabled
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: GCSState.worklightOn = !GCSState.worklightOn
                            }
                        }

                        // ALS auto toggle
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 4
                            color: GCSState.alsAutoEnabled ? Qt.rgba(Theme.accentYellow.r, Theme.accentYellow.g, Theme.accentYellow.b, 0.10) : Theme.bgSecondary
                            border.color: GCSState.alsAutoEnabled ? Theme.accentYellow : Theme.border
                            border.width: GCSState.alsAutoEnabled ? 2 : 1

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2
                                Text {
                                    text: "AMBIENT"
                                    color: GCSState.alsAutoEnabled ? Theme.accentYellow : Theme.textSecondary
                                    font.pixelSize: 10
                                    font.weight: Font.SemiBold
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Text {
                                    text: GCSState.alsAutoEnabled ? "AUTO" : "MANUAL"
                                    color: GCSState.alsAutoEnabled ? Theme.accentYellow : Theme.textDisabled
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: GCSState.alsAutoEnabled = !GCSState.alsAutoEnabled
                            }
                        }
                    }

                    // ── WORKLIGHT COLOR ─────────────────────────────────────
                    RowLayout {
                        spacing: 6
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        visible: true

                        Text {
                            text: "COLOR"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSectionLabel
                            font.weight: Font.Medium
                            Layout.preferredWidth: 50
                        }

                        Repeater {
                            model: [
                                { label: "W",  r: 255, g: 255, b: 255 },
                                { label: "WW", r: 255, g: 200, b: 120 },
                                { label: "R",  r: 255, g: 0,   b: 0   },
                                { label: "G",  r: 0,   g: 255, b: 0   },
                                { label: "B",  r: 0,   g: 100, b: 255 },
                                { label: "A",  r: 255, g: 180, b: 0   }
                            ]

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 4

                                property color swatchColor: Qt.rgba(modelData.r / 255, modelData.g / 255, modelData.b / 255, 1.0)
                                property bool active: GCSState.worklightColor.r === modelData.r / 255
                                                      && GCSState.worklightColor.g === modelData.g / 255
                                                      && GCSState.worklightColor.b === modelData.b / 255

                            color: Qt.rgba(swatchColor.r, swatchColor.g, swatchColor.b, 0.15)
                            border.color: active ? Theme.accentYellow : swatchColor
                            border.width: active ? 2 : 1

                                // Color dot
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 14; height: 14; radius: 7
                                    color: parent.swatchColor
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: GCSState.worklightColor = Qt.rgba(modelData.r / 255, modelData.g / 255, modelData.b / 255, 1.0)
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 24
                        spacing: 6

                        Repeater {
                            model: [
                                { lbl: "SCREEN L", val: GCSState.brightnessScreenL },
                                { lbl: "SCREEN R", val: GCSState.brightnessScreenR },
                                { lbl: "LED STRIP", val: GCSState.brightnessLed },
                                { lbl: "TFT SCREEN", val: GCSState.brightnessTft },
                                { lbl: "BUTTON LEDS", val: GCSState.brightnessBtnLeds }
                            ]
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 3
                                color: Theme.bgSecondary
                                border.color: Theme.border
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.lbl + " " + modelData.val
                                    color: Theme.textSecondary
                                    font.pixelSize: 8
                                    font.family: "monospace"
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                radius: 4
                color: Theme.bgElevated
                border.color: Theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text {
                        text: "BRIGHTNESS"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSectionLabel
                        font.weight: Font.Medium
                        Layout.preferredWidth: 82
                    }

                    Slider {
                        id: masterSlider
                        from: 0; to: 100; stepSize: 1
                        value: GCSState.brightnessScreenL
                        Layout.fillWidth: true

                        // Custom track
                        background: Item {
                            x: masterSlider.leftPadding
                            y: masterSlider.topPadding + masterSlider.availableHeight / 2 - height / 2
                            implicitWidth: 200; implicitHeight: 6
                            width: masterSlider.availableWidth; height: 6

                            Rectangle {
                                width: parent.width; height: parent.height; radius: 3
                                color: Theme.bgSecondary
                                border.color: Theme.border; border.width: 1
                            }
                            Rectangle {
                                width: masterSlider.visualPosition * parent.width
                                height: parent.height; radius: 3
                                color: Theme.accentBlue; opacity: 0.85
                            }
                        }

                        // Custom handle
                        handle: Rectangle {
                            x: masterSlider.leftPadding + masterSlider.visualPosition * (masterSlider.availableWidth - width)
                            y: masterSlider.topPadding + masterSlider.availableHeight / 2 - height / 2
                            width: 18; height: 18; radius: 9
                            color: Theme.accentYellow
                            border.color: Theme.bgPrimary; border.width: 2

                            Rectangle {
                                anchors.centerIn: parent
                                width: 6; height: 6; radius: 3
                                color: Theme.bgPrimary; opacity: 0.5
                            }
                            scale: masterSlider.pressed ? 0.84 : 1.0
                            Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }
                        }

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
                            GCSState.brightnessScreenR = Math.min(100, Math.round(brightnessRatios[1] * v))
                            GCSState.brightnessLed     = Math.min(100, Math.round(brightnessRatios[2] * v))
                            GCSState.brightnessTft     = Math.min(100, Math.round(brightnessRatios[3] * v))
                            GCSState.brightnessBtnLeds = Math.min(100, Math.round(brightnessRatios[4] * v))
                        }
                    }

                    Text {
                        text: Math.round(masterSlider.value) + "%"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontUnit
                        font.family: "monospace"
                        Layout.preferredWidth: 44
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                radius: 4
                color: Theme.bgElevated
                border.color: Theme.border
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Text {
                        text: "QUICK LAYOUTS"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSectionLabel
                        font.weight: Font.SemiBold
                        font.letterSpacing: 0.6
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: ["PILOT", "MAP", "DEBUG", "POWER", "CLEAN"]
                            Rectangle {
                                Layout.fillWidth: true
                                height: 26
                                radius: 3
                                property bool selected: panel.quickLayoutMode === modelData
                                color: selected ? Qt.rgba(Theme.accentYellow.r, Theme.accentYellow.g, Theme.accentYellow.b, 0.12) : Theme.bgSecondary
                                border.color: selected ? Theme.accentYellow : Theme.border
                                border.width: selected ? 2 : 1
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: parent.selected ? Theme.accentYellow : Theme.textSecondary
                                    font.pixelSize: 9
                                    font.weight: Font.SemiBold
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: panel.quickLayoutMode = modelData
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                radius: 4
                color: Theme.bgElevated
                border.color: Theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text {
                        text: "PROCESSOR " + GCSState.tempCpuPi.toFixed(0) + "°"
                        color: GCSState.tempCpuPi < 80 ? Theme.textSecondary : Theme.statusWarn
                        font.pixelSize: Theme.fontUnit
                        font.family: "monospace"
                    }
                    Text {
                        text: "MEMORY " + GCSState.memPercent + "%"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontUnit
                        font.family: "monospace"
                    }
                    Text {
                        text: "DISK USAGE " + GCSState.diskPercent + "%"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontUnit
                        font.family: "monospace"
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: panel.quickLayoutMode
                        color: Theme.accentBlue
                        font.pixelSize: Theme.fontSectionLabel
                        font.weight: Font.SemiBold
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
