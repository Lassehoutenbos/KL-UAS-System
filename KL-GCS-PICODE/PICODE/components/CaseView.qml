import QtQuick
import QtQuick.Layouts
import PICODE

Item {
    id: caseView
    implicitHeight: 250

    // Track min/max since component creation
    property double minCpuPi:  9999
    property double maxCpuPi:  -9999
    property double minCaseA:  9999
    property double maxCaseA:  -9999
    property double minCaseB:  9999
    property double maxCaseB:  -9999
    property double minCaseC:  9999
    property double maxCaseC:  -9999
    property double minCaseD:  9999
    property double maxCaseD:  -9999

    property int selectedSensor: -1  // -1 = none, 0-4 = sensor index

    // Sensor data model
    readonly property var sensors: [
        { label: "PI CPU",       temp: GCSState.tempCpuPi, valid: true,                                                    cx: 0.50, cy: 0.38 },
        { label: "PCB POWER",    temp: GCSState.tempCaseA, valid: !isNaN(GCSState.tempCaseA) && GCSState.tempCaseA > -274, cx: 0.25, cy: 0.52 },
        { label: "RASPBERRY PI", temp: GCSState.tempCaseB, valid: !isNaN(GCSState.tempCaseB) && GCSState.tempCaseB > -274, cx: 0.50, cy: 0.48 },
        { label: "CHARGER",      temp: GCSState.tempCaseC, valid: !isNaN(GCSState.tempCaseC) && GCSState.tempCaseC > -274, cx: 0.30, cy: 0.62 },
        { label: "VRX MODULE",   temp: GCSState.tempCaseD, valid: !isNaN(GCSState.tempCaseD) && GCSState.tempCaseD > -274, cx: 0.72, cy: 0.55 }
    ]

    function sensorLevel(temp, valid) {
        if (!valid) return -1
        if (temp < 60) return 0
        if (temp < 80) return 1
        return 2
    }

    function levelColor(level) {
        if (level === 2) return Theme.statusCrit
        if (level === 1) return Theme.statusWarn
        if (level === 0) return Theme.statusOk
        return Theme.textDisabled
    }

    // Update min/max tracking
    Connections {
        target: GCSState
        function onSensorChanged() {
            if (GCSState.tempCpuPi > caseView.maxCpuPi) caseView.maxCpuPi = GCSState.tempCpuPi
            if (GCSState.tempCpuPi < caseView.minCpuPi) caseView.minCpuPi = GCSState.tempCpuPi

            if (!isNaN(GCSState.tempCaseA) && GCSState.tempCaseA > -274) {
                if (GCSState.tempCaseA > caseView.maxCaseA) caseView.maxCaseA = GCSState.tempCaseA
                if (GCSState.tempCaseA < caseView.minCaseA) caseView.minCaseA = GCSState.tempCaseA
            }
            if (!isNaN(GCSState.tempCaseB) && GCSState.tempCaseB > -274) {
                if (GCSState.tempCaseB > caseView.maxCaseB) caseView.maxCaseB = GCSState.tempCaseB
                if (GCSState.tempCaseB < caseView.minCaseB) caseView.minCaseB = GCSState.tempCaseB
            }
            if (!isNaN(GCSState.tempCaseC) && GCSState.tempCaseC > -274) {
                if (GCSState.tempCaseC > caseView.maxCaseC) caseView.maxCaseC = GCSState.tempCaseC
                if (GCSState.tempCaseC < caseView.minCaseC) caseView.minCaseC = GCSState.tempCaseC
            }
            if (!isNaN(GCSState.tempCaseD) && GCSState.tempCaseD > -274) {
                if (GCSState.tempCaseD > caseView.maxCaseD) caseView.maxCaseD = GCSState.tempCaseD
                if (GCSState.tempCaseD < caseView.minCaseD) caseView.minCaseD = GCSState.tempCaseD
            }

            caseCanvas.requestPaint()
        }
    }

    function getMinMax(index) {
        switch (index) {
        case 0: return { min: minCpuPi, max: maxCpuPi }
        case 1: return { min: minCaseA, max: maxCaseA }
        case 2: return { min: minCaseB, max: maxCaseB }
        case 3: return { min: minCaseC, max: maxCaseC }
        case 4: return { min: minCaseD, max: maxCaseD }
        }
        return { min: 0, max: 0 }
    }

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
                var lvl = sensorLevel(s.temp, s.valid)
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
            text: modelData.valid ? modelData.temp.toFixed(0) + "°C" : "—"
            color: levelColor(sensorLevel(modelData.temp, modelData.valid))
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
                      ? sensors[selectedSensor].temp.toFixed(1) + "°C"
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
                    text: mm.min < 9999 ? ("MIN " + mm.min.toFixed(0) + "°") : "MIN —"
                    color: Theme.textSecondary
                    font.pixelSize: 9
                    font.family: "monospace"
                }
                Text {
                    property var mm: visible && selectedSensor >= 0 ? getMinMax(selectedSensor) : { min: 0, max: 0 }
                    text: mm.max > -9999 ? ("MAX " + mm.max.toFixed(0) + "°") : "MAX —"
                    color: Theme.textSecondary
                    font.pixelSize: 9
                    font.family: "monospace"
                }
                Text {
                    property int lvl: visible && selectedSensor >= 0
                                      ? sensorLevel(sensors[selectedSensor].temp, sensors[selectedSensor].valid) : -1
                    text: lvl === 0 ? "OK" : (lvl === 1 ? "WARN" : (lvl === 2 ? "CRIT" : "—"))
                    color: levelColor(lvl)
                    font.pixelSize: 9
                    font.weight: Font.Bold
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
