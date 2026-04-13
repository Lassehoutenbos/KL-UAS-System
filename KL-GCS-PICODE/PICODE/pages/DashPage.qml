import QtQuick
import QtQuick.Layouts
import PICODE

Rectangle {
    color: Theme.bgPrimary

    // Helper: warning level → label string
    function warnLabel(v) { return v === 0 ? "OK" : (v === 1 ? "WARN" : "CRIT") }
    function warnColor(v) { return v === 0 ? Theme.statusOk : (v === 1 ? Theme.statusWarn : Theme.statusCrit) }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── LEFT: Drone stats ─────────────────────────────────────── 280px ──
        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: Theme.bgSecondary
            border.color: Theme.border; border.width: 1

            ColumnLayout {
                anchors { fill: parent; margins: 0 }
                spacing: 0

                // DRONE section header
                Item {
                    Layout.fillWidth: true; height: 30
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentYellow }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "DRONE"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Flight data rows
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: 10
                    spacing: 6

                    Repeater {
                        model: [
                            { lbl: "ALT",  val: GCSState.altitude.toFixed(1),     unit: "m" },
                            { lbl: "SPD",  val: GCSState.groundSpeed.toFixed(1),  unit: "m/s" },
                            { lbl: "HDG",  val: GCSState.heading.toFixed(0),      unit: "°" },
                            { lbl: "VSPD", val: (GCSState.verticalSpeed >= 0 ? "+" : "") + GCSState.verticalSpeed.toFixed(1), unit: "m/s" }
                        ]
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: modelData.lbl;  color: Theme.textDisabled;  font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 42; font.letterSpacing: 0.4 }
                            Text { text: modelData.val;  color: Theme.textPrimary;   font.pixelSize: Theme.fontValueSmall;   font.weight: Font.Bold;    Layout.fillWidth: true; font.family: "monospace" }
                            Text { text: modelData.unit; color: Theme.textDisabled;  font.pixelSize: Theme.fontUnit }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // MODE section header
                Item {
                    Layout.fillWidth: true; height: 26
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "MODE & ARM"
                        color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.margins: 10; spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "MODE";  color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 42; font.letterSpacing: 0.4 }
                        Text { text: GCSState.flightMode; color: Theme.accentYellow; font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace" }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 6
                        Text { text: "ARMED"; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 42; font.letterSpacing: 0.4 }
                        StatusDot { level: GCSState.droneArmed ? 2 : 0 }
                        Text {
                            text: GCSState.droneArmed ? "ARMED" : "DISARMED"
                            color: GCSState.droneArmed ? Theme.statusCrit : Theme.statusOk
                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                            Behavior on color { ColorAnimation { duration: 200 } }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // BATTERY section header
                Item {
                    Layout.fillWidth: true; height: 26
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "POWER"
                        color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.margins: 10; spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "BAT";  color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 42; font.letterSpacing: 0.4 }
                        Text {
                            text: GCSState.batteryVoltage.toFixed(1) + "V"
                            color: GCSState.batteryPercent < 20 ? Theme.statusCrit
                                 : GCSState.batteryPercent < 40 ? Theme.statusWarn
                                 : Theme.textPrimary
                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                        }
                        Item { Layout.fillWidth: true }
                        Text { text: GCSState.batteryPercent + "%"; color: Theme.textSecondary; font.pixelSize: Theme.fontSectionLabel; font.family: "monospace" }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "EXT";  color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 42; font.letterSpacing: 0.4 }
                        Text { text: GCSState.extVoltage.toFixed(1) + "V"; color: Theme.textPrimary; font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace" }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

        // ── CENTER: Attitude HUD ──────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors { fill: parent; margins: 8 }
                spacing: 6

                // HUD Canvas
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Canvas {
                        id: hudCanvas
                        anchors.fill: parent

                        onPaint: {
                            var ctx = getContext("2d")
                            var w = width, h = height
                            var cx = w / 2, cy = h / 2
                            var pitch = GCSState.pitch
                            var roll  = GCSState.roll

                            ctx.clearRect(0, 0, w, h)

                            ctx.save()
                            ctx.beginPath(); ctx.rect(0, 0, w, h); ctx.clip()
                            ctx.translate(cx, cy)
                            ctx.rotate(-roll * Math.PI / 180)

                            var pOff = -pitch * 3

                            // Sky
                            ctx.fillStyle = "#0a2236"
                            ctx.fillRect(-w, -h * 2, w * 2, h * 2 + pOff)
                            // Ground
                            ctx.fillStyle = "#2e1a0a"
                            ctx.fillRect(-w, pOff, w * 2, h * 2)
                            // Horizon line
                            ctx.strokeStyle = "#7fd6ff"
                            ctx.lineWidth = 2
                            ctx.beginPath()
                            ctx.moveTo(-w, pOff); ctx.lineTo(w, pOff)
                            ctx.stroke()

                            // Pitch ladder
                            for (var deg = -30; deg <= 30; deg += 5) {
                                if (deg === 0) continue
                                var ly = pOff - deg * 3
                                var lw = (deg % 10 === 0) ? 52 : 26
                                ctx.strokeStyle = "rgba(255,255,255,0.6)"
                                ctx.lineWidth = (deg % 10 === 0) ? 1.5 : 1
                                ctx.beginPath()
                                ctx.moveTo(-lw / 2, ly); ctx.lineTo(lw / 2, ly)
                                ctx.stroke()
                                if (deg % 10 === 0) {
                                    ctx.fillStyle = "rgba(255,255,255,0.6)"
                                    ctx.font = "bold 9px monospace"
                                    ctx.textAlign = "left"
                                    ctx.fillText(deg.toString(), lw / 2 + 5, ly + 3)
                                }
                            }
                            ctx.restore()

                            // Aircraft reticle
                            ctx.strokeStyle = "#e3d049"
                            ctx.lineWidth = 2.5
                            ctx.beginPath(); ctx.moveTo(cx - 60, cy); ctx.lineTo(cx - 20, cy); ctx.stroke()
                            ctx.beginPath(); ctx.moveTo(cx - 20, cy); ctx.lineTo(cx - 20, cy + 12); ctx.stroke()
                            ctx.beginPath(); ctx.moveTo(cx + 20, cy); ctx.lineTo(cx + 60, cy); ctx.stroke()
                            ctx.beginPath(); ctx.moveTo(cx + 20, cy); ctx.lineTo(cx + 20, cy + 12); ctx.stroke()
                            ctx.fillStyle = "#e3d049"
                            ctx.beginPath(); ctx.arc(cx, cy, 4, 0, 2 * Math.PI); ctx.fill()

                            // Roll arc
                            var rad = Math.min(cx, cy) - 14
                            ctx.strokeStyle = "rgba(127,214,255,0.4)"
                            ctx.lineWidth = 1
                            ctx.beginPath()
                            ctx.arc(cx, cy, rad, -Math.PI * 0.75, -Math.PI * 0.25)
                            ctx.stroke()

                            // Roll pointer
                            ctx.save()
                            ctx.translate(cx, cy)
                            ctx.rotate(-roll * Math.PI / 180)
                            ctx.fillStyle = "#7fd6ff"
                            ctx.beginPath()
                            ctx.moveTo(0, -rad); ctx.lineTo(-5, -(rad - 10)); ctx.lineTo(5, -(rad - 10))
                            ctx.closePath(); ctx.fill()
                            ctx.restore()
                        }

                        Connections {
                            target: GCSState
                            function onDroneStateChanged() { hudCanvas.requestPaint() }
                        }
                    }

                    // Thin accent border around HUD
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: Qt.rgba(0.5, 0.84, 1.0, 0.18)
                        border.width: 1
                        radius: 3
                    }
                    // Corner brackets
                    Rectangle { anchors { top: parent.top; left: parent.left } width: 14; height: 1.5; color: Theme.accentBlue; opacity: 0.6 }
                    Rectangle { anchors { top: parent.top; left: parent.left } width: 1.5; height: 14; color: Theme.accentBlue; opacity: 0.6 }
                    Rectangle { anchors { top: parent.top; right: parent.right } width: 14; height: 1.5; color: Theme.accentBlue; opacity: 0.6 }
                    Rectangle { anchors { top: parent.top; right: parent.right } width: 1.5; height: 14; color: Theme.accentBlue; opacity: 0.6 }
                    Rectangle { anchors { bottom: parent.bottom; left: parent.left } width: 14; height: 1.5; color: Theme.accentBlue; opacity: 0.6 }
                    Rectangle { anchors { bottom: parent.bottom; left: parent.left } width: 1.5; height: 14; color: Theme.accentBlue; opacity: 0.6 }
                    Rectangle { anchors { bottom: parent.bottom; right: parent.right } width: 14; height: 1.5; color: Theme.accentBlue; opacity: 0.6 }
                    Rectangle { anchors { bottom: parent.bottom; right: parent.right } width: 1.5; height: 14; color: Theme.accentBlue; opacity: 0.6 }
                }

                // Attitude readouts
                RowLayout {
                    Layout.fillWidth: true
                    height: 44
                    spacing: 0

                    Repeater {
                        model: [
                            { lbl: "PITCH", val: (GCSState.pitch >= 0 ? "+" : "") + GCSState.pitch.toFixed(1) + "°" },
                            { lbl: "ROLL",  val: (GCSState.roll  >= 0 ? "+" : "") + GCSState.roll.toFixed(1)  + "°" },
                            { lbl: "YAW",   val: GCSState.yaw.toFixed(0) + "°" }
                        ]
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text { Layout.alignment: Qt.AlignHCenter; text: modelData.lbl; color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.Medium; font.letterSpacing: 0.6 }
                            Text { Layout.alignment: Qt.AlignHCenter; text: modelData.val; color: Theme.accentBlue;   font.pixelSize: Theme.fontValueMedium; font.weight: Font.Bold; font.family: "monospace" }
                        }
                    }
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

        // ── RIGHT: GCS state ──────────────────────────────────────── 280px ──
        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: Theme.bgSecondary
            border.color: Theme.border; border.width: 1

            ColumnLayout {
                anchors { fill: parent; margins: 0 }
                spacing: 0

                // GCS section header
                Item {
                    Layout.fillWidth: true; height: 30
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "GCS"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.margins: 10; spacing: 7

                    Repeater {
                        model: [
                            { lbl: "LINK",  dot: GCSState.mavlinkConnected ? 0 : 2,
                              val: GCSState.mavlinkConnected ? "OK" : "NO LINK",
                              col: GCSState.mavlinkConnected ? Theme.statusOk : Theme.statusCrit },
                            { lbl: "GPS",   dot: GCSState.gpsSats >= 6 ? 0 : (GCSState.gpsSats > 0 ? 1 : 2),
                              val: GCSState.gpsSats + "  " + GCSState.gpsFixType,
                              col: Theme.textPrimary },
                            { lbl: "ALS",   dot: 0,
                              val: GCSState.alsLux.toFixed(0) + " lux",
                              col: Theme.textPrimary },
                            { lbl: "TEMP",  dot: GCSState.tempCpuPi < 60 ? 0 : (GCSState.tempCpuPi < 80 ? 1 : 2),
                              val: GCSState.tempCpuPi.toFixed(0) + "°C",
                              col: Theme.textPrimary }
                        ]
                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            StatusDot { level: modelData.dot }
                            Text { text: modelData.lbl; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 38; font.letterSpacing: 0.4 }
                            Text { text: modelData.val; color: modelData.col;     font.pixelSize: Theme.fontValueSmall;   font.weight: Font.SemiBold; Layout.fillWidth: true; font.family: "monospace" }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // WARNINGS header
                Item {
                    Layout.fillWidth: true; height: 26
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.statusCrit; opacity: GCSState.anyWarningActive ? 1.0 : 0.3 }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "WARNINGS"
                        color: GCSState.anyWarningActive ? Theme.statusWarn : Theme.textDisabled
                        font.pixelSize: 10; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Warning rows as mini badge-style entries
                ColumnLayout {
                    Layout.fillWidth: true; Layout.margins: 10; spacing: 5

                    Repeater {
                        model: [
                            { lbl: "TEMP",  level: GCSState.warnTemp },
                            { lbl: "SIG",   level: GCSState.warnSignal },
                            { lbl: "DRONE", level: GCSState.warnDrone },
                            { lbl: "GPS",   level: GCSState.warnGps },
                            { lbl: "LINK",  level: GCSState.warnLink },
                            { lbl: "NET",   level: GCSState.warnNetwork }
                        ]

                        Rectangle {
                            Layout.fillWidth: true; height: 28; radius: 3
                            color: modelData.level === 0
                                   ? "transparent"
                                   : Qt.rgba(1, modelData.level === 1 ? 0.7 : 0.36, modelData.level === 1 ? 0.28 : 0.36, 0.07)
                            border.color: modelData.level === 0 ? Theme.border : warnColor(modelData.level)
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 200 } }

                            RowLayout {
                                anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
                                spacing: 6
                                StatusDot { level: modelData.level }
                                Text { text: modelData.lbl; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 40; font.letterSpacing: 0.4 }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: warnLabel(modelData.level)
                                    color: warnColor(modelData.level)
                                    font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold; font.family: "monospace"; font.letterSpacing: 0.5
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
