import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import PICODE

Rectangle {
    color: Theme.bgPrimary

    property bool hudVisible: true
    property bool recording: false
    property int  recordSeconds: 0
    property real zoomLevel: 1.0

    Timer {
        id: recordTimer
        interval: 1000
        repeat: true
        running: recording
        onTriggered: recordSeconds++
    }

    function formatRecordTime(s) {
        var h = Math.floor(s / 3600)
        var m = Math.floor((s % 3600) / 60)
        var sec = s % 60
        return (h > 0 ? h.toString().padStart(2, '0') + ":" : "")
             + m.toString().padStart(2, '0') + ":"
             + sec.toString().padStart(2, '0')
    }

    CaptureSession {
        id: captureSession
        camera: Camera {
            id: cam
            active: true
        }
        videoOutput: videoOut
        recorder: MediaRecorder {
            id: recorder
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Video area with HUD overlay ──────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            VideoOutput {
                id: videoOut
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectCrop
            }

            // No-signal fallback
            Rectangle {
                anchors.fill: parent
                color: "#060809"
                visible: !cam.active

                Text {
                    anchors.centerIn: parent
                    text: "NO VIDEO SIGNAL"
                    color: Theme.textDisabled
                    font.pixelSize: Theme.fontValueMedium
                    font.weight: Font.SemiBold
                }
            }

            // ── Crosshair Reticle ──
            Item {
                anchors.centerIn: parent
                width: 120; height: 120
                visible: GCSState.crosshairActive
                z: 5

                // Horizontal line
                Rectangle { anchors.centerIn: parent; width: 80; height: 1.5; color: Qt.rgba(1, 0.3, 0.3, 0.8) }
                // Vertical line
                Rectangle { anchors.centerIn: parent; width: 1.5; height: 80; color: Qt.rgba(1, 0.3, 0.3, 0.8) }
                // Center dot
                Rectangle { anchors.centerIn: parent; width: 6; height: 6; radius: 3; color: Qt.rgba(1, 0.3, 0.3, 0.9) }
                // Corner brackets
                Rectangle { x: 10; y: 10; width: 16; height: 1.5; color: Qt.rgba(1, 0.3, 0.3, 0.6) }
                Rectangle { x: 10; y: 10; width: 1.5; height: 16; color: Qt.rgba(1, 0.3, 0.3, 0.6) }
                Rectangle { x: parent.width - 26; y: 10; width: 16; height: 1.5; color: Qt.rgba(1, 0.3, 0.3, 0.6) }
                Rectangle { x: parent.width - 11.5; y: 10; width: 1.5; height: 16; color: Qt.rgba(1, 0.3, 0.3, 0.6) }
                Rectangle { x: 10; y: parent.height - 11.5; width: 16; height: 1.5; color: Qt.rgba(1, 0.3, 0.3, 0.6) }
                Rectangle { x: 10; y: parent.height - 26; width: 1.5; height: 16; color: Qt.rgba(1, 0.3, 0.3, 0.6) }
                Rectangle { x: parent.width - 26; y: parent.height - 11.5; width: 16; height: 1.5; color: Qt.rgba(1, 0.3, 0.3, 0.6) }
                Rectangle { x: parent.width - 11.5; y: parent.height - 26; width: 1.5; height: 16; color: Qt.rgba(1, 0.3, 0.3, 0.6) }
            }

            // ── HUD Overlay ──
            Item {
                anchors.fill: parent
                visible: hudVisible

                // Top-left: ALT
                Rectangle {
                    anchors { top: parent.top; left: parent.left; margins: 10 }
                    color: Qt.rgba(0, 0, 0, 0.4)
                    radius: 3
                    width: altHudText.width + 16
                    height: 28
                    Text {
                        id: altHudText
                        anchors.centerIn: parent
                        text: "ALT " + GCSState.altitude.toFixed(0).padStart(3, '0') + "m"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontValueSmall
                        font.weight: Font.SemiBold
                    }
                }

                // Top-right: HDG
                Rectangle {
                    anchors { top: parent.top; right: parent.right; margins: 10 }
                    color: Qt.rgba(0, 0, 0, 0.4)
                    radius: 3
                    width: hdgHudText.width + 16
                    height: 28
                    Text {
                        id: hdgHudText
                        anchors.centerIn: parent
                        text: "HDG " + GCSState.heading.toFixed(0).padStart(3, '0') + "°"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontValueSmall
                        font.weight: Font.SemiBold
                    }
                }

                // Bottom-left: SPD
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; margins: 10 }
                    color: Qt.rgba(0, 0, 0, 0.4)
                    radius: 3
                    width: spdHudText.width + 16
                    height: 28
                    Text {
                        id: spdHudText
                        anchors.centerIn: parent
                        text: "SPD " + GCSState.groundSpeed.toFixed(1) + "m/s"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontValueSmall
                        font.weight: Font.SemiBold
                    }
                }

                // Bottom-center: camera controls
                Row {
                    anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; margins: 10 }
                    spacing: 8

                    Rectangle {
                        width: 44; height: 44; radius: 4; color: Qt.rgba(0, 0, 0, 0.5)
                        border.color: Theme.border; border.width: 1
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.textPrimary; font.pixelSize: 18; font.weight: Font.Bold }
                        MouseArea { anchors.fill: parent; onClicked: zoomLevel = Math.min(8.0, zoomLevel + 0.5) }
                    }
                    Rectangle {
                        width: 44; height: 44; radius: 4; color: Qt.rgba(0, 0, 0, 0.5)
                        border.color: Theme.border; border.width: 1
                        Text { anchors.centerIn: parent; text: "−"; color: Theme.textPrimary; font.pixelSize: 18; font.weight: Font.Bold }
                        MouseArea { anchors.fill: parent; onClicked: zoomLevel = Math.max(1.0, zoomLevel - 0.5) }
                    }
                    Rectangle {
                        width: 44; height: 44; radius: 22; color: recording ? Theme.statusCrit : Qt.rgba(0, 0, 0, 0.5)
                        border.color: recording ? Theme.statusCrit : Theme.border; border.width: 2
                        Text { anchors.centerIn: parent; text: "●"; color: Theme.textPrimary; font.pixelSize: 16 }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (recording) {
                                    recorder.stop()
                                    recording = false
                                    recordSeconds = 0
                                } else {
                                    recorder.record()
                                    recording = true
                                }
                            }
                        }
                    }
                    Rectangle {
                        width: 44; height: 44; radius: 4; color: Qt.rgba(0, 0, 0, 0.5)
                        border.color: Theme.border; border.width: 1
                        Text { anchors.centerIn: parent; text: "⊡"; color: Theme.textPrimary; font.pixelSize: 16 }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: captureSession.imageCapture.captureToFile("")
                        }
                    }
                }

                // Bottom-right: BAT
                Rectangle {
                    anchors { bottom: parent.bottom; right: parent.right; margins: 10 }
                    color: Qt.rgba(0, 0, 0, 0.4)
                    radius: 3
                    width: batHudText.width + 16
                    height: 28
                    Text {
                        id: batHudText
                        anchors.centerIn: parent
                        text: "BAT " + GCSState.batteryVoltage.toFixed(1) + "V"
                        color: GCSState.batteryPercent < 20 ? Theme.statusCrit : Theme.textPrimary
                        font.pixelSize: Theme.fontValueSmall
                        font.weight: Font.SemiBold
                    }
                }
            }
        }

        // ── Bottom controls strip ──────────────────────────────────────── 68px ──
        Rectangle {
            Layout.fillWidth: true
            height: 68
            color: Theme.bgSecondary
            border.color: Theme.border
            border.width: 1

            RowLayout {
                anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                spacing: 12

                // Zoom slider
                Text { text: "ZOOM"; color: Theme.textSecondary; font.pixelSize: Theme.fontUnit }
                Slider {
                    id: zoomSlider
                    from: 1.0; to: 8.0; stepSize: 0.5
                    value: zoomLevel
                    Layout.preferredWidth: 160
                    onMoved: zoomLevel = value
                }
                Text { text: zoomLevel.toFixed(1) + "×"; color: Theme.textPrimary; font.pixelSize: Theme.fontUnit; Layout.preferredWidth: 36 }

                Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

                // Record status
                StatusDot { level: recording ? 2 : -1 }
                Text {
                    text: recording ? "REC " + formatRecordTime(recordSeconds) : "STANDBY"
                    color: recording ? Theme.statusCrit : Theme.textSecondary
                    font.pixelSize: Theme.fontUnit
                    font.weight: Font.Medium
                    Layout.preferredWidth: 90
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

                // Snapshot button
                Rectangle {
                    width: 100; height: 44; radius: 4
                    color: Theme.bgElevated; border.color: Theme.border; border.width: 1
                    Text { anchors.centerIn: parent; text: "SNAP"; color: Theme.textPrimary; font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold }
                    MouseArea { anchors.fill: parent; onClicked: captureSession.imageCapture.captureToFile("") }
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

                // Overlay toggle
                Rectangle {
                    width: 120; height: 44; radius: 4
                    color: hudVisible ? Theme.bgElevated : Theme.bgSecondary
                    border.color: hudVisible ? Theme.accentYellow : Theme.border
                    border.width: hudVisible ? 2 : 1
                    Text { anchors.centerIn: parent; text: hudVisible ? "OVERLAY ON" : "OVERLAY OFF"; color: hudVisible ? Theme.accentYellow : Theme.textSecondary; font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold }
                    MouseArea { anchors.fill: parent; onClicked: hudVisible = !hudVisible }
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

                // Crosshair / tracking toggle
                Rectangle {
                    width: 100; height: 44; radius: 4
                    color: GCSState.crosshairActive ? Qt.rgba(1, 0.3, 0.3, 0.12) : Theme.bgSecondary
                    border.color: GCSState.crosshairActive ? Theme.statusCrit : Theme.border
                    border.width: GCSState.crosshairActive ? 2 : 1
                    Text { anchors.centerIn: parent; text: GCSState.crosshairActive ? "TRACK ON" : "TRACK"; color: GCSState.crosshairActive ? Theme.statusCrit : Theme.textSecondary; font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold }
                    MouseArea { anchors.fill: parent; onClicked: GCSState.crosshairActive = !GCSState.crosshairActive }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }
}
