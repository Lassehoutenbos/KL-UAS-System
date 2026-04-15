import QtQuick
import QtQuick.Layouts
import PICODE

Item {
    id: caseView
    implicitHeight: 250
    signal sensorDetailsRequested(var sensor)
    property int selectedSensor: -1  // -1 = none, 0-4 = sensor index
    property var minTemps: []
    property var maxTemps: []

    function valueForSensorKey(sensorKey) {
        if (sensorKey === "tempCpuPi") return GCSState.tempCpuPi
        if (sensorKey === "tempCaseA") return GCSState.tempCaseA
        if (sensorKey === "tempCaseB") return GCSState.tempCaseB
        if (sensorKey === "tempCaseC") return GCSState.tempCaseC
        if (sensorKey === "tempCaseD") return GCSState.tempCaseD
        if (sensorKey === "batteryVoltage") return GCSState.batteryVoltage
        if (sensorKey === "extVoltage") return GCSState.extVoltage
        if (sensorKey === "alsLux") return GCSState.alsLux
        if (sensorKey === "brightnessScreenL") return GCSState.brightnessScreenL
        if (sensorKey === "brightnessScreenR") return GCSState.brightnessScreenR
        if (sensorKey === "brightnessLed") return GCSState.brightnessLed
        if (sensorKey === "brightnessTft") return GCSState.brightnessTft
        if (sensorKey === "brightnessBtnLeds") return GCSState.brightnessBtnLeds
        if (sensorKey === "mavlinkConnected") return GCSState.mavlinkConnected
        if (sensorKey === "picoConnected") return GCSState.picoConnected
        if (sensorKey === "droneArmed") return GCSState.droneArmed
        if (sensorKey === "keyUnlocked") return GCSState.keyUnlocked
        if (sensorKey === "worklightOn") return GCSState.worklightOn
        if (sensorKey === "anyWarningActive") return GCSState.anyWarningActive
        if (sensorKey === "warnTemp") return GCSState.warnTemp
        if (sensorKey === "warnSignal") return GCSState.warnSignal
        if (sensorKey === "warnDrone") return GCSState.warnDrone
        if (sensorKey === "warnGps") return GCSState.warnGps
        if (sensorKey === "warnLink") return GCSState.warnLink
        if (sensorKey === "warnNetwork") return GCSState.warnNetwork
        if (sensorKey === "peripheralCount") return GCSState.peripherals.length
        return NaN
    }

    function sensorValid(type, sensorKey, value) {
        if (type === "bool") return value !== undefined && value !== null
        if (type === "text") return value !== undefined && value !== null && ("" + value).length > 0
        if (sensorKey === "tempCpuPi") return !isNaN(value)
        if (type === "temp") return !isNaN(value) && value > -274
        if (type === "number" || type === "voltage" || type === "percent") return !isNaN(value)
        return value !== undefined && value !== null && !isNaN(value)
    }

    readonly property var sensors: {
        var cfg = GCSState.caseTwinHotspots
        var out = []
        for (var i = 0; i < cfg.length; i++) {
            var row = cfg[i]
            var sensorKey = row.sensorKey !== undefined ? row.sensorKey : row.sensor_key
            var t = valueForSensorKey(sensorKey)
            var metricType = row.metricType !== undefined ? row.metricType : (row.type !== undefined ? row.type : "temp")
            var okMax = row.okMax !== undefined ? row.okMax : (row.ok_max !== undefined ? row.ok_max : 60)
            var warnMax = row.warnMax !== undefined ? row.warnMax : (row.warn_max !== undefined ? row.warn_max : 80)
            var warnMin = row.warnMin !== undefined ? row.warnMin : row.warn_min
            var critMin = row.critMin !== undefined ? row.critMin : row.crit_min
            var decimals = row.decimals !== undefined ? row.decimals : (metricType === "temp" || metricType === "voltage" ? 1 : 0)
            var unit = row.unit !== undefined ? row.unit : (metricType === "temp" ? "°C" : (metricType === "voltage" ? "V" : ""))
            out.push({
                id: row.id,
                label: row.label,
                sensorKey: sensorKey,
                value: t,
                valid: sensorValid(metricType, sensorKey, t),
                cx: row.xNorm !== undefined ? row.xNorm : row.x_norm,
                cy: row.yNorm !== undefined ? row.yNorm : row.y_norm,
                type: metricType,
                unit: unit,
                decimals: decimals,
                okMax: okMax,
                warnMax: warnMax,
                warnMin: warnMin,
                critMin: critMin,
                trueLabel: row.trueLabel !== undefined ? row.trueLabel : row.true_label,
                falseLabel: row.falseLabel !== undefined ? row.falseLabel : row.false_label,
                invert: row.invert === true
            })
        }
        return out
    }

    function resetMinMaxBuffers() {
        var mins = []
        var maxs = []
        for (var i = 0; i < sensors.length; i++) {
            mins.push(9999)
            maxs.push(-9999)
        }
        minTemps = mins
        maxTemps = maxs
    }

    function sensorLevel(sensor) {
        if (!sensor.valid) return -1
        var value = sensor.value

        if (sensor.type === "bool") {
            var active = !!value
            if (sensor.invert === true) active = !active
            return active ? 0 : 2
        }

        if (sensor.critMin !== undefined && value < sensor.critMin) return 2
        if (sensor.warnMin !== undefined && value < sensor.warnMin) return 1
        if (sensor.warnMax !== undefined && value >= sensor.warnMax) return 2
        if (sensor.okMax !== undefined && value >= sensor.okMax) return 1
        return 0
    }

    function sensorDisplayValue(sensor) {
        if (!sensor.valid) return "—"

        if (sensor.type === "bool") {
            var active = !!sensor.value
            if (sensor.invert === true) active = !active
            return active ? (sensor.trueLabel || "ON") : (sensor.falseLabel || "OFF")
        }

        if (typeof sensor.value === "number")
            return sensor.value.toFixed(sensor.decimals !== undefined ? sensor.decimals : 0) + (sensor.unit || "")

        return "" + sensor.value
    }

    function levelColor(level) {
        if (level === 2) return Theme.statusCrit
        if (level === 1) return Theme.statusWarn
        if (level === 0) return Theme.statusOk
        return Theme.textDisabled
    }

    Connections {
        target: GCSState
        function onSensorChanged() {
            var mins = minTemps.slice()
            var maxs = maxTemps.slice()
            while (mins.length < sensors.length) mins.push(9999)
            while (maxs.length < sensors.length) maxs.push(-9999)

            for (var i = 0; i < sensors.length; i++) {
                var s = sensors[i]
                if (s.valid) {
                    if (typeof s.value === "number") {
                        if (s.value < mins[i]) mins[i] = s.value
                        if (s.value > maxs[i]) maxs[i] = s.value
                    }
                }
            }
            minTemps = mins
            maxTemps = maxs
            caseCanvas.requestPaint()
        }

        function onCaseTwinConfigChanged() {
            resetMinMaxBuffers()
            if (selectedSensor >= sensors.length) selectedSensor = -1
            caseCanvas.requestPaint()
        }
    }

    function getMinMax(index) {
        var minV = index < minTemps.length ? minTemps[index] : 9999
        var maxV = index < maxTemps.length ? maxTemps[index] : -9999
        return { min: minV, max: maxV }
    }

    Component.onCompleted: resetMinMaxBuffers()

    Canvas {
        id: caseCanvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            var w = width
            var h = height
            ctx.clearRect(0, 0, w, h)

            // Isometric case dimensions
            var caseW  = w * 0.60
            var caseH  = h * 0.35
            var depth  = h * 0.18
            var left   = (w - caseW) / 2
            var top    = h * 0.18
            var isoX   = depth * 0.8
            var isoY   = depth * 0.5

            // ── RIGHT FACE ──────────────────────────────────────────
            ctx.beginPath()
            ctx.moveTo(left + caseW, top + isoY)
            ctx.lineTo(left + caseW + isoX, top)
            ctx.lineTo(left + caseW + isoX, top + caseH - isoY)
            ctx.lineTo(left + caseW, top + caseH)
            ctx.closePath()
            ctx.fillStyle = Qt.rgba(Theme.bgPrimary.r, Theme.bgPrimary.g, Theme.bgPrimary.b, 1.0)
            ctx.fill()
            ctx.strokeStyle = Theme.border
            ctx.lineWidth = 1.5
            ctx.stroke()

            // ── TOP FACE ────────────────────────────────────────────
            ctx.beginPath()
            ctx.moveTo(left, top + isoY)
            ctx.lineTo(left + isoX, top)
            ctx.lineTo(left + caseW + isoX, top)
            ctx.lineTo(left + caseW, top + isoY)
            ctx.closePath()
            ctx.fillStyle = Qt.rgba(Theme.bgSecondary.r, Theme.bgSecondary.g, Theme.bgSecondary.b, 1.0)
            ctx.fill()
            ctx.stroke()

            // LED strip on top edge (accent line)
            ctx.beginPath()
            ctx.moveTo(left + isoX + 10, top + 2)
            ctx.lineTo(left + caseW + isoX - 10, top + 2)
            ctx.strokeStyle = GCSState.worklightOn
                ? Qt.rgba(GCSState.worklightColor.r, GCSState.worklightColor.g, GCSState.worklightColor.b, 0.7)
                : Theme.border
            ctx.lineWidth = 3
            ctx.stroke()

            // ── FRONT FACE ──────────────────────────────────────────
            ctx.beginPath()
            ctx.moveTo(left, top + isoY)
            ctx.lineTo(left + caseW, top + isoY)
            ctx.lineTo(left + caseW, top + caseH)
            ctx.lineTo(left, top + caseH)
            ctx.closePath()
            ctx.fillStyle = Qt.rgba(Theme.bgSecondary.r, Theme.bgSecondary.g, Theme.bgSecondary.b, 0.92)
            ctx.fill()
            ctx.strokeStyle = Theme.border
            ctx.lineWidth = 1.5
            ctx.stroke()

            // Front panel subtle accent frame
            ctx.strokeStyle = Qt.rgba(Theme.accentBlue.r, Theme.accentBlue.g, Theme.accentBlue.b, 0.18)
            ctx.lineWidth = 1
            ctx.strokeRect(left + 3, top + isoY + 3, caseW - 6, caseH - isoY - 6)

            // Screen outlines (two screens)
            var scrW = caseW * 0.38
            var scrH = caseH * 0.70
            var scrY = top + isoY + (caseH - isoY - scrH) / 2
            var scrGap = caseW * 0.04
            var scr1X = left + scrGap
            var scr2X = left + caseW - scrGap - scrW

            ctx.strokeStyle = Theme.border
            ctx.lineWidth = 1
            // Screen L
            ctx.strokeRect(scr1X, scrY, scrW, scrH)
            // Screen R
            ctx.strokeRect(scr2X, scrY, scrW, scrH)

            // Screen labels
            ctx.fillStyle = Theme.textDisabled
            ctx.font = "9px monospace"
            ctx.textAlign = "center"
            ctx.fillText("L", scr1X + scrW / 2, scrY + scrH + 12)
            ctx.fillText("R", scr2X + scrW / 2, scrY + scrH + 12)

            // Center control panel area
            var panelX = scr1X + scrW + 4
            var panelW = scr2X - panelX - 4
            ctx.strokeStyle = Theme.border
            ctx.strokeRect(panelX, scrY, panelW, scrH)
            ctx.fillStyle = Theme.textDisabled
            ctx.fillText("CTRL", panelX + panelW / 2, scrY + scrH / 2 + 3)

            // Battery voltage on front face
            ctx.fillStyle = Theme.accentBlue
            ctx.font = "bold 10px monospace"
            ctx.textAlign = "left"
            ctx.fillText(GCSState.batteryVoltage.toFixed(1) + "V", left + 8, top + caseH - 6)

            if (GCSState.extVoltage > 1.0) {
                ctx.fillStyle = Theme.statusOk
                ctx.fillText("EXT " + GCSState.extVoltage.toFixed(1) + "V", left + caseW - 90, top + caseH - 6)
            }

            // ── SENSOR DOTS ─────────────────────────────────────────
            for (var i = 0; i < sensors.length; i++) {
                var s = sensors[i]
                var sx = s.cx * w
                var sy = s.cy * h
                var lvl = sensorLevel(s)
                var col = levelColor(lvl)

                // Outer ring
                ctx.beginPath()
                ctx.arc(sx, sy, 8, 0, Math.PI * 2)
                ctx.fillStyle = Qt.rgba(col.r || 0.38, col.g || 0.83, col.b || 0.56, 0.15)
                ctx.fill()

                // Inner dot
                ctx.beginPath()
                ctx.arc(sx, sy, 4, 0, Math.PI * 2)
                ctx.fillStyle = col
                ctx.fill()

                // Selection ring
                if (selectedSensor === i) {
                    ctx.beginPath()
                    ctx.arc(sx, sy, 12, 0, Math.PI * 2)
                    ctx.strokeStyle = Theme.accentYellow
                    ctx.lineWidth = 2
                    ctx.stroke()
                }
            }
        }
    }

    // Tap areas for each sensor
    Repeater {
        model: sensors

        MouseArea {
            x: modelData.cx * caseView.width - 14
            y: modelData.cy * caseView.height - 14
            width: 28; height: 28

            onClicked: {
                if (selectedSensor === index) selectedSensor = -1
                else selectedSensor = index
                caseCanvas.requestPaint()
            }
        }
    }

    // Sensor temperature labels
    Repeater {
        model: sensors

        Text {
            x: modelData.cx * caseView.width + 12
            y: modelData.cy * caseView.height - 7
            text: sensorDisplayValue(modelData)
            color: levelColor(sensorLevel(modelData))
            font.pixelSize: 10
            font.weight: Font.Bold
            font.family: "monospace"
        }
    }

    // Sensor name labels
    Repeater {
        model: sensors

        Text {
            x: modelData.cx * caseView.width + 12
            y: modelData.cy * caseView.height + 5
            text: modelData.label
            color: Theme.textDisabled
            font.pixelSize: 8
            font.weight: Font.Medium
            font.letterSpacing: 0.3
        }
    }

    // ── DETAIL OVERLAY ──────────────────────────────────────────────────
    Rectangle {
        id: detailOverlay
        visible: selectedSensor >= 0 && selectedSensor < sensors.length
        width: 200; height: 100; radius: 6
        color: Theme.bgElevated
        border.color: Theme.accentYellow
        border.width: 1

        x: visible ? Math.min(Math.max(sensors[selectedSensor].cx * caseView.width - 100, 8), caseView.width - 208) : 0
        y: visible ? Math.max(sensors[selectedSensor].cy * caseView.height - 110, 4) : 0

        Behavior on x { NumberAnimation { duration: 100 } }
        Behavior on y { NumberAnimation { duration: 100 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            Text {
                text: visible ? sensors[selectedSensor].label : ""
                color: Theme.accentYellow
                font.pixelSize: Theme.fontSectionLabel
                font.weight: Font.SemiBold
            }

            Text {
                text: visible && sensors[selectedSensor].valid
                      ? sensorDisplayValue(sensors[selectedSensor])
                      : "—"
                color: Theme.textPrimary
                font.pixelSize: 22
                font.weight: Font.Bold
                font.family: "monospace"
            }

            RowLayout {
                spacing: 12
                Text {
                    property var mm: visible && selectedSensor >= 0 ? getMinMax(selectedSensor) : { min: 0, max: 0 }
                    property int dec: visible && selectedSensor >= 0 && sensors[selectedSensor] ? (sensors[selectedSensor].decimals !== undefined ? sensors[selectedSensor].decimals : 0) : 0
                    property string unitText: visible && selectedSensor >= 0 && sensors[selectedSensor] ? (sensors[selectedSensor].unit || "") : ""
                    text: mm.min < 9999
                          ? ("MIN " + mm.min.toFixed(dec) + unitText)
                          : "MIN —"
                    color: Theme.textSecondary
                    font.pixelSize: 9
                    font.family: "monospace"
                }
                Text {
                    property var mm: visible && selectedSensor >= 0 ? getMinMax(selectedSensor) : { min: 0, max: 0 }
                    property int dec: visible && selectedSensor >= 0 && sensors[selectedSensor] ? (sensors[selectedSensor].decimals !== undefined ? sensors[selectedSensor].decimals : 0) : 0
                    property string unitText: visible && selectedSensor >= 0 && sensors[selectedSensor] ? (sensors[selectedSensor].unit || "") : ""
                    text: mm.max > -9999
                          ? ("MAX " + mm.max.toFixed(dec) + unitText)
                          : "MAX —"
                    color: Theme.textSecondary
                    font.pixelSize: 9
                    font.family: "monospace"
                }
                Text {
                    property int lvl: visible && selectedSensor >= 0
                                      ? sensorLevel(sensors[selectedSensor]) : -1
                    text: lvl === 0 ? "OK" : (lvl === 1 ? "WARN" : (lvl === 2 ? "CRIT" : "—"))
                    color: levelColor(lvl)
                    font.pixelSize: 9
                    font.weight: Font.Bold
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 22
                radius: 4
                color: Theme.bgSecondary
                border.color: Theme.border
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "MEER INFO"
                    color: Theme.textPrimary
                    font.pixelSize: 10
                    font.weight: Font.SemiBold
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (detailOverlay.visible && selectedSensor >= 0 && selectedSensor < sensors.length)
                            caseView.sensorDetailsRequested(sensors[selectedSensor])
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: { selectedSensor = -1; caseCanvas.requestPaint() }
        }
    }

    // Dismiss on background tap
    MouseArea {
        anchors.fill: parent
        z: -1
        onClicked: {
            if (selectedSensor >= 0) {
                selectedSensor = -1
                caseCanvas.requestPaint()
            }
        }
    }
}
