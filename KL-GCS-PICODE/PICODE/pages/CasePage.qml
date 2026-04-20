import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PICODE

Rectangle {
    color: Theme.bgPrimary
    readonly property int sliderLabelWidth: 120
    readonly property int sliderValueWidth: 52
    property bool brightnessExpanded: false

    // Compute current TFT mode from GCSState (mirrors picolink updateTftMode logic)
    readonly property int tftMode: {
        if (GCSState.batteryPercent < 15)          return 4
        if (GCSState.anyWarningActive)             return 2
        if (!GCSState.keyUnlocked)                 return 3
        if (GCSState.peripherals.length > 0)       return 5
        return 1
    }

    property bool screensLinked: true
    property bool use3D: false
    property string configPath: "case_twin_config.json"
    property string configStatus: ""
    property var selectedTwinSensor: ({})
    property bool sensorModalVisible: false

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
                RowLayout {
                    anchors { fill: parent; leftMargin: 11; rightMargin: 12 }
                    spacing: 8

                    Text {
                        text: "BRIGHTNESS"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: brightnessExpanded ? "▼" : "▶"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSectionLabel
                        font.weight: Font.SemiBold
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: brightnessExpanded = !brightnessExpanded
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 12
                spacing: 6

                // MASTER
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 4
                    color: Theme.bgSecondary
                    border.color: Theme.border
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Text {
                            text: "MASTER"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontUnit
                            font.weight: Font.Medium
                            Layout.preferredWidth: sliderLabelWidth
                        }

                        Slider {
                            id: masterSlider
                            from: 0; to: 100; stepSize: 1
                            value: GCSState.brightnessScreenL
                            Layout.fillWidth: true

                            // Match SliderRow visual style
                            background: Item {
                                x: masterSlider.leftPadding
                                y: masterSlider.topPadding + masterSlider.availableHeight / 2 - height / 2
                                implicitWidth: 200; implicitHeight: 6
                                width: masterSlider.availableWidth; height: 6

                                Rectangle {
                                    width: parent.width; height: parent.height; radius: 3
                                    color: Theme.bgElevated
                                    border.color: Theme.border; border.width: 1
                                }
                                Rectangle {
                                    width: masterSlider.visualPosition * parent.width
                                    height: parent.height; radius: 3
                                    color: Theme.accentBlue
                                    opacity: 0.85
                                }
                            }

                            handle: Rectangle {
                                x: masterSlider.leftPadding + masterSlider.visualPosition * (masterSlider.availableWidth - width)
                                y: masterSlider.topPadding + masterSlider.availableHeight / 2 - height / 2
                                width: 18; height: 18; radius: 9
                                color: Theme.accentYellow
                                border.color: Theme.bgPrimary; border.width: 2

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 6; height: 6; radius: 3
                                    color: Theme.bgPrimary
                                    opacity: 0.5
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
                            font.weight: Font.SemiBold
                            font.family: "monospace"
                            Layout.preferredWidth: 52
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }

                ColumnLayout {
                    visible: brightnessExpanded
                    Layout.fillWidth: true
                    spacing: 6

                // SCREENS + link toggle
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 82
                    radius: 4
                    color: Theme.bgSecondary
                    border.color: Theme.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        anchors.topMargin: 6
                        anchors.bottomMargin: 6
                        spacing: 4

                        SliderRow {
                            label: screensLinked ? "SCREENS" : "SCREEN L"
                            labelWidth: sliderLabelWidth
                            valueWidth: sliderValueWidth
                            value: GCSState.brightnessScreenL
                            Layout.fillWidth: true
                            onSliderMoved: function(v) {
                                GCSState.brightnessScreenL = v
                                if (screensLinked) GCSState.brightnessScreenR = v
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 28

                            Rectangle {
                                width: 220
                                anchors.right: parent.right
                                anchors.rightMargin: sliderValueWidth + 8
                                height: 28
                                radius: 14
                                color: screensLinked
                                       ? Qt.rgba(Theme.accentYellow.r, Theme.accentYellow.g, Theme.accentYellow.b, 0.14)
                                       : Theme.bgElevated
                                border.color: screensLinked ? Theme.accentYellow : Theme.border
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 8

                                    Rectangle {
                                        width: 10; height: 10; radius: 5
                                        color: screensLinked ? Theme.accentYellow : Theme.textDisabled
                                    }

                                    Text {
                                        text: screensLinked ? "SCREENS LINKED" : "SCREENS UNLINKED"
                                        color: screensLinked ? Theme.accentYellow : Theme.textSecondary
                                        font.pixelSize: 10
                                        font.weight: Font.SemiBold
                                    }

                                    Item { Layout.fillWidth: true }

                                    Text {
                                        text: screensLinked ? "LINK" : "UNLINK"
                                        color: screensLinked ? Theme.accentYellow : Theme.textDisabled
                                        font.pixelSize: 10
                                        font.weight: Font.Medium
                                        font.letterSpacing: 0.4
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: screensLinked = !screensLinked
                                }
                            }
                        }
                    }
                }

                // SCREEN R — only when unlinked
                Rectangle {
                    visible: !screensLinked
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    radius: 4
                    color: Theme.bgSecondary
                    border.color: Theme.border
                    border.width: 1
                    SliderRow {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        label: "SCREEN R"
                        labelWidth: sliderLabelWidth
                        valueWidth: sliderValueWidth
                        value: GCSState.brightnessScreenR
                        onSliderMoved: function(v) { GCSState.brightnessScreenR = v }
                    }
                }

                Repeater {
                    model: [
                        { lbl: "LED STRIP", val: GCSState.brightnessLed, setFn: function(v) { GCSState.brightnessLed = v } },
                        { lbl: "TFT BL", val: GCSState.brightnessTft, setFn: function(v) { GCSState.brightnessTft = v } },
                        { lbl: "BTN LEDS", val: GCSState.brightnessBtnLeds, setFn: function(v) { GCSState.brightnessBtnLeds = v } }
                    ]
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        radius: 4
                        color: Theme.bgSecondary
                        border.color: Theme.border
                        border.width: 1
                        SliderRow {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            label: modelData.lbl
                            labelWidth: sliderLabelWidth
                            valueWidth: sliderValueWidth
                            value: modelData.val
                            onSliderMoved: function(v) { modelData.setFn(v) }
                        }
                    }
                }

                // ALS auto-brightness toggle
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    radius: 4
                    color: Theme.bgSecondary
                    border.color: Theme.border
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Text {
                            text: "AMBIENT LIGHT AUTO-BRIGHTNESS"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSectionLabel
                            font.weight: Font.Medium
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            width: 120; height: 34; radius: 4
                            color: GCSState.alsAutoEnabled ? Theme.accentYellow : Theme.bgElevated
                            border.color: GCSState.alsAutoEnabled ? Theme.accentYellow : Theme.border
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: GCSState.alsAutoEnabled ? "ENABLED" : "DISABLED"
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
            }

            // ── CASE OVERVIEW ────────────────────────────────────────────────
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: 4 }
            Item {
                Layout.fillWidth: true; height: 32
                Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 3; color: Theme.accentBlue }
                RowLayout {
                    anchors { fill: parent; leftMargin: 11; rightMargin: 8 }
                    spacing: 8
                    Text {
                        text: "CASE OVERVIEW"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                    Item { Layout.fillWidth: true }
                    // 2D / 3D toggle
                    Repeater {
                        model: ["2D", "3D"]
                        Rectangle {
                            required property string modelData
                            required property int index
                            width: 36; height: 22; radius: 3
                            color: (use3D ? index === 1 : index === 0) ? Theme.accentBlue : Theme.bgElevated
                            border.color: (use3D ? index === 1 : index === 0) ? Theme.accentBlue : Theme.border
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: (use3D ? index === 1 : index === 0) ? Theme.bgPrimary : Theme.textSecondary
                                font.pixelSize: Theme.fontSectionLabel
                                font.weight: Font.SemiBold
                            }
                            MouseArea { anchors.fill: parent; onClicked: use3D = (index === 1) }
                        }
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            CaseView {
                id: caseOverview
                visible: !use3D
                Layout.fillWidth: true
                Layout.preferredHeight: 250
                Layout.margins: 4
                onSensorDetailsRequested: function(sensor) {
                    selectedTwinSensor = sensor
                    sensorModalVisible = true
                }
            }

            Loader {
                active: use3D
                visible: use3D
                Layout.fillWidth: true
                Layout.preferredHeight: 350
                Layout.margins: 4
                sourceComponent: CaseView3D {
                    onSensorDetailsRequested: function(sensor) {
                        selectedTwinSensor = sensor
                        sensorModalVisible = true
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 12
                spacing: 6

                TextField {
                    Layout.fillWidth: true
                    text: configPath
                    placeholderText: "Pad naar case twin config JSON"
                    onTextChanged: configPath = text
                    background: Rectangle {
                        radius: 4
                        color: Theme.bgSecondary
                        border.color: Theme.border
                        border.width: 1
                    }
                }

                Rectangle {
                    width: 86; height: 34; radius: 4
                    color: Theme.bgElevated
                    border.color: Theme.border
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "LOAD"; color: Theme.textPrimary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.SemiBold }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            var ok = GCSState.loadCaseTwinConfig(configPath)
                            configStatus = ok ? "Config geladen: " + configPath : ("Load fout: " + GCSState.caseTwinConfigLastError)
                        }
                    }
                }

                Rectangle {
                    width: 86; height: 34; radius: 4
                    color: Theme.bgElevated
                    border.color: Theme.border
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "SAVE"; color: Theme.textPrimary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.SemiBold }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            var ok = GCSState.saveCaseTwinConfig(configPath)
                            configStatus = ok ? "Config opgeslagen: " + configPath : ("Save fout: " + GCSState.caseTwinConfigLastError)
                        }
                    }
                }

                Rectangle {
                    width: 86; height: 34; radius: 4
                    color: Theme.bgElevated
                    border.color: Theme.border
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "RESET"; color: Theme.textPrimary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.SemiBold }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            GCSState.resetCaseTwinConfig()
                            configStatus = "Config teruggezet naar defaults"
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: 6
                text: configStatus.length > 0 ? configStatus : "Tip: gebruik LOAD/SAVE met JSON config om hotspot-locaties te uploaden."
                color: configStatus.indexOf("fout") >= 0 ? Theme.statusCrit : Theme.textDisabled
                font.pixelSize: Theme.fontSectionLabel
                wrapMode: Text.Wrap
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
                        { lbl: "PCB POWER",    temp: GCSState.tempCaseA,  valid: !isNaN(GCSState.tempCaseA) && GCSState.tempCaseA > -274 },
                        { lbl: "RASPBERRY PI", temp: GCSState.tempCaseB,  valid: !isNaN(GCSState.tempCaseB) && GCSState.tempCaseB > -274 },
                        { lbl: "CHARGER",      temp: GCSState.tempCaseC,  valid: !isNaN(GCSState.tempCaseC) && GCSState.tempCaseC > -274 },
                        { lbl: "VRX MODULE",   temp: GCSState.tempCaseD,  valid: !isNaN(GCSState.tempCaseD) && GCSState.tempCaseD > -274 }
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
                            Text { text: modelData.lbl; color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.Medium; Layout.preferredWidth: 90; font.letterSpacing: 0.3 }
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

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.45)
        visible: sensorModalVisible
        z: 20

        MouseArea {
            anchors.fill: parent
            onClicked: sensorModalVisible = false
        }
    }

    Rectangle {
        visible: sensorModalVisible
        z: 21
        width: Math.min(parent.width - 120, 480)
        height: 220
        radius: 8
        color: Theme.bgElevated
        border.color: Theme.accentYellow
        border.width: 1
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            Text {
                text: selectedTwinSensor && selectedTwinSensor.label ? selectedTwinSensor.label : "SENSOR"
                color: Theme.accentYellow
                font.pixelSize: Theme.fontPageTitle
                font.weight: Font.SemiBold
            }
            Text {
                text: selectedTwinSensor && selectedTwinSensor.valid
                      ? (selectedTwinSensor.type === "bool"
                         ? ((selectedTwinSensor.invert ? !selectedTwinSensor.value : selectedTwinSensor.value)
                            ? (selectedTwinSensor.trueLabel || "ON")
                            : (selectedTwinSensor.falseLabel || "OFF"))
                         : (typeof selectedTwinSensor.value === "number"
                            ? selectedTwinSensor.value.toFixed(selectedTwinSensor.decimals !== undefined ? selectedTwinSensor.decimals : 0)
                              + (selectedTwinSensor.unit || "")
                            : selectedTwinSensor.value))
                      : "—"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontValueMedium
                font.weight: Font.Bold
                font.family: "monospace"
            }
            Text {
                text: "KEY: " + (selectedTwinSensor && selectedTwinSensor.sensorKey ? selectedTwinSensor.sensorKey : "—")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSectionLabel
                font.family: "monospace"
            }
            Text {
                text: {
                    if (!selectedTwinSensor || selectedTwinSensor.type === "bool")
                        return "THRESHOLD: BOOL STATE"
                    var unit = selectedTwinSensor.unit || ""
                    var okTxt = selectedTwinSensor.okMax !== undefined ? selectedTwinSensor.okMax + unit : "—"
                    var warnTxt = selectedTwinSensor.warnMax !== undefined ? selectedTwinSensor.warnMax + unit : "—"
                    var warnMinTxt = selectedTwinSensor.warnMin !== undefined ? selectedTwinSensor.warnMin + unit : "—"
                    var critMinTxt = selectedTwinSensor.critMin !== undefined ? selectedTwinSensor.critMin + unit : "—"
                    return "THRESHOLD: okMax=" + okTxt + " warnMax=" + warnTxt
                           + " warnMin=" + warnMinTxt + " critMin=" + critMinTxt
                }
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSectionLabel
            }

            Item { Layout.fillHeight: true }

            Rectangle {
                Layout.alignment: Qt.AlignRight
                width: 100
                height: 34
                radius: 4
                color: Theme.bgSecondary
                border.color: Theme.border
                border.width: 1
                Text { anchors.centerIn: parent; text: "SLUIT"; color: Theme.textPrimary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.SemiBold }
                MouseArea { anchors.fill: parent; onClicked: sensorModalVisible = false }
            }
        }
    }
}
