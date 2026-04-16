import QtQuick
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.AssetUtils
import PICODE

Item {
    id: root
    implicitHeight: 350

    signal sensorDetailsRequested(var sensor)
    property int selectedSensor: -1

    // Orbit camera state — drag to rotate, scroll to zoom
    property real orbitYaw:      0.0
    property real orbitPitch:    0.0
    property real orbitDistance: 1.0

    // ── Sensor data helpers (mirrors CaseView.qml) ────────────────────────
    function valueForSensorKey(key) {
        if (key === "tempCpuPi")         return GCSState.tempCpuPi
        if (key === "tempCaseA")         return GCSState.tempCaseA
        if (key === "tempCaseB")         return GCSState.tempCaseB
        if (key === "tempCaseC")         return GCSState.tempCaseC
        if (key === "tempCaseD")         return GCSState.tempCaseD
        if (key === "batteryVoltage")    return GCSState.batteryVoltage
        if (key === "extVoltage")        return GCSState.extVoltage
        if (key === "alsLux")            return GCSState.alsLux
        if (key === "brightnessScreenL") return GCSState.brightnessScreenL
        if (key === "brightnessScreenR") return GCSState.brightnessScreenR
        if (key === "brightnessLed")     return GCSState.brightnessLed
        if (key === "brightnessTft")     return GCSState.brightnessTft
        if (key === "brightnessBtnLeds") return GCSState.brightnessBtnLeds
        if (key === "mavlinkConnected")  return GCSState.mavlinkConnected
        if (key === "picoConnected")     return GCSState.picoConnected
        if (key === "droneArmed")        return GCSState.droneArmed
        if (key === "keyUnlocked")       return GCSState.keyUnlocked
        if (key === "worklightOn")       return GCSState.worklightOn
        if (key === "anyWarningActive")  return GCSState.anyWarningActive
        if (key === "warnTemp")          return GCSState.warnTemp
        if (key === "warnSignal")        return GCSState.warnSignal
        if (key === "warnDrone")         return GCSState.warnDrone
        if (key === "warnGps")           return GCSState.warnGps
        if (key === "warnLink")          return GCSState.warnLink
        if (key === "warnNetwork")       return GCSState.warnNetwork
        if (key === "peripheralCount")   return GCSState.peripherals.length
        return NaN
    }

    function sensorValid(type, key, value) {
        if (type === "bool") return value !== undefined && value !== null
        if (type === "text") return value !== undefined && value !== null && ("" + value).length > 0
        if (key === "tempCpuPi") return !isNaN(value)
        if (type === "temp")    return !isNaN(value) && value > -274
        if (type === "number" || type === "voltage" || type === "percent") return !isNaN(value)
        return value !== undefined && value !== null && !isNaN(value)
    }

    function sensorLevel(sensor) {
        if (!sensor.valid) return -1
        var v = sensor.value
        if (sensor.type === "bool") {
            var active = !!v
            if (sensor.invert === true) active = !active
            return active ? 0 : 2
        }
        if (sensor.critMin !== undefined && v < sensor.critMin) return 2
        if (sensor.warnMin !== undefined && v < sensor.warnMin) return 1
        if (sensor.warnMax !== undefined && v >= sensor.warnMax) return 2
        if (sensor.okMax  !== undefined && v >= sensor.okMax)   return 1
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

    readonly property var sensors: {
        var cfg = GCSState.caseTwinHotspots
        var out = []
        for (var i = 0; i < cfg.length; i++) {
            var row = cfg[i]
            var key = row.sensorKey !== undefined ? row.sensorKey : row.sensor_key
            var t   = valueForSensorKey(key)
            var mt  = row.metricType !== undefined ? row.metricType : (row.type !== undefined ? row.type : "temp")
            var okMax   = row.okMax   !== undefined ? row.okMax   : (row.ok_max   !== undefined ? row.ok_max   : 60)
            var warnMax = row.warnMax !== undefined ? row.warnMax : (row.warn_max !== undefined ? row.warn_max : 80)
            var warnMin = row.warnMin !== undefined ? row.warnMin : row.warn_min
            var critMin = row.critMin !== undefined ? row.critMin : row.crit_min
            var dec  = row.decimals !== undefined ? row.decimals : (mt === "temp" || mt === "voltage" ? 1 : 0)
            var unit = row.unit !== undefined ? row.unit : (mt === "temp" ? "°C" : (mt === "voltage" ? "V" : ""))
            out.push({
                id:         row.id,
                label:      row.label,
                sensorKey:  key,
                value:      t,
                valid:      sensorValid(mt, key, t),
                type:       mt,
                unit:       unit,
                decimals:   dec,
                okMax:      okMax,
                warnMax:    warnMax,
                warnMin:    warnMin,
                critMin:    critMin,
                trueLabel:  row.trueLabel  !== undefined ? row.trueLabel  : row.true_label,
                falseLabel: row.falseLabel !== undefined ? row.falseLabel : row.false_label,
                invert:     row.invert === true,
                x3d:        row.x3d !== undefined ? row.x3d : 0,
                y3d:        row.y3d !== undefined ? row.y3d : 0,
                z3d:        row.z3d !== undefined ? row.z3d : 0
            })
        }
        return out
    }

    // ── 3D Scene ─────────────────────────────────────────────────────────
    View3D {
        id: view3d
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: Theme.bgPrimary
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.Medium
        }

        // Orbit rig: camera child offset on local +Z, rig rotates around model center.
        // eulerRotation(-90, 0, 0) points local +Z toward world +Y (front of case).
        Node {
            id: cameraRig
            position: Qt.vector3d(0, -0.124, 0.180)
            eulerRotation: Qt.vector3d(-90.0 + root.orbitPitch, root.orbitYaw, 0)

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, root.orbitDistance)
                clipNear: 0.01
                clipFar:  10.0
                fieldOfView: 40
            }
        }

        // Key light (front-top)
        DirectionalLight {
            eulerRotation: Qt.vector3d(-45, 20, 0)
            brightness: 1.0
            color: Qt.rgba(1.0, 0.97, 0.92, 1.0)
        }

        // Fill light (back-side, cooler)
        DirectionalLight {
            eulerRotation: Qt.vector3d(30, -140, 0)
            brightness: 0.35
            color: Qt.rgba(0.75, 0.85, 1.0, 1.0)
        }

        // The case model — loaded at runtime via QtQuick3D.AssetUtils
        RuntimeLoader {
            id: caseModel
            source: "qrc:/qt/qml/PICODE/assets/models/case_v13.glb"
            onStatusChanged: {
                if (status === RuntimeLoader.Error)
                    console.warn("CaseView3D model error:", errorString)
                else if (status === RuntimeLoader.Ready)
                    console.log("CaseView3D model loaded OK")
            }
        }
    }

    // ── 2D Sensor Overlays ────────────────────────────────────────────────
    // Labels are positioned by projecting 3D world coords to viewport space.
    Repeater {
        model: root.sensors

        Item {
            id: hotspotItem
            required property var modelData
            required property int index

            property vector3d vp: camera.mapToViewport(
                Qt.vector3d(modelData.x3d, modelData.y3d, modelData.z3d))

            // vp.x/y in [0,1], vp.z > 0 means in front of camera
            x: vp.x * view3d.width  - 14
            y: vp.y * view3d.height - 14
            width: 28; height: 28
            visible: vp.z > 0

            // Coloured dot
            Rectangle {
                anchors.centerIn: parent
                width:  root.selectedSensor === index ? 16 : 10
                height: width; radius: width / 2
                color: root.levelColor(root.sensorLevel(modelData))
                border.color: root.selectedSensor === index ? Theme.accentYellow : "transparent"
                border.width: 2

                Behavior on width { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                // Pulse ring on warn/crit
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width + 8; height: width; radius: width / 2
                    color: "transparent"
                    border.color: root.levelColor(root.sensorLevel(hotspotItem.modelData))
                    border.width: 1
                    opacity: root.sensorLevel(hotspotItem.modelData) > 0 ? 0.4 : 0
                }
            }

            // Value label
            Text {
                x: 16; y: -7
                text: root.sensorDisplayValue(modelData)
                color: root.levelColor(root.sensorLevel(modelData))
                font.pixelSize: 10
                font.weight: Font.Bold
                font.family: "monospace"
            }

            // Name label
            Text {
                x: 16; y: 6
                text: modelData.label
                color: Theme.textDisabled
                font.pixelSize: 8
                font.letterSpacing: 0.3
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.selectedSensor = (root.selectedSensor === index) ? -1 : index
            }
        }
    }

    // ── Selected sensor detail popup ─────────────────────────────────────
    Rectangle {
        id: detailPopup
        visible: root.selectedSensor >= 0 && root.selectedSensor < root.sensors.length
        width: 200; height: 100; radius: 6
        color: Theme.bgElevated
        border.color: Theme.accentYellow
        border.width: 1
        z: 10

        property var sel: visible ? root.sensors[root.selectedSensor] : null
        property vector3d vp: sel
            ? camera.mapToViewport(Qt.vector3d(sel.x3d, sel.y3d, sel.z3d))
            : Qt.vector3d(0.5, 0.5, 1)

        x: Math.min(Math.max(vp.x * view3d.width - 100, 8), root.width - 208)
        y: Math.max(vp.y * view3d.height - 112, 4)

        Behavior on x { NumberAnimation { duration: 100 } }
        Behavior on y { NumberAnimation { duration: 100 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            Text {
                text: detailPopup.sel ? detailPopup.sel.label : ""
                color: Theme.accentYellow
                font.pixelSize: Theme.fontSectionLabel
                font.weight: Font.SemiBold
            }

            Text {
                text: detailPopup.sel ? root.sensorDisplayValue(detailPopup.sel) : "—"
                color: Theme.textPrimary
                font.pixelSize: 22
                font.weight: Font.Bold
                font.family: "monospace"
            }

            Text {
                property int lvl: detailPopup.sel ? root.sensorLevel(detailPopup.sel) : -1
                text: lvl === 0 ? "OK" : (lvl === 1 ? "WARN" : (lvl === 2 ? "CRIT" : "—"))
                color: root.levelColor(lvl)
                font.pixelSize: 9
                font.weight: Font.Bold
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 22
                radius: 4
                color: Theme.bgSecondary
                border.color: Theme.border; border.width: 1
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
                        if (detailPopup.sel)
                            root.sensorDetailsRequested(detailPopup.sel)
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.selectedSensor = -1
        }
    }

    // ── Orbit drag & scroll ───────────────────────────────────────────────
    MouseArea {
        anchors.fill: parent
        z: -1
        property real lastX: 0
        property real lastY: 0

        onPressed:  { lastX = mouseX; lastY = mouseY }
        onPositionChanged: {
            if (pressed) {
                root.orbitYaw   += (mouseX - lastX) * 0.4
                root.orbitPitch  = Math.max(-80, Math.min(80,
                    root.orbitPitch - (mouseY - lastY) * 0.3))
                lastX = mouseX; lastY = mouseY
            }
        }
        onWheel: root.orbitDistance = Math.max(0.3, Math.min(3.0,
            root.orbitDistance - wheel.angleDelta.y * 0.001))
    }

    // ── Controls overlay ─────────────────────────────────────────────────
    RowLayout {
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        spacing: 4

        Rectangle {
            width: 60; height: 24; radius: 4
            color: Theme.bgElevated; border.color: Theme.border; border.width: 1
            Text { anchors.centerIn: parent; text: "RESET"; color: Theme.textSecondary; font.pixelSize: 9; font.weight: Font.SemiBold }
            MouseArea {
                anchors.fill: parent
                onClicked: { root.orbitYaw = 0; root.orbitPitch = 0; root.orbitDistance = 1.0 }
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        text: "DRAG — orbit   SCROLL — zoom"
        color: Theme.textDisabled
        font.pixelSize: 8
        font.letterSpacing: 0.3
    }

    Connections {
        target: GCSState
        function onSensorChanged()        { root.sensorsChanged() }
        function onCaseTwinConfigChanged() { root.sensorsChanged() }
    }
}
