import QtQuick
import QtQuick.Layouts
import PICODE

Rectangle {
    color: Theme.bgSecondary

    // Bottom separator
    Rectangle {
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 1; color: Theme.border
    }

    signal quickPanelToggled()

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── LINK ─────────────────────────────────────────────────────────────
        Item {
            Layout.preferredWidth: 120
            Layout.fillHeight: true

            RowLayout {
                anchors { fill: parent; leftMargin: 10 }
                spacing: 6
                StatusDot { level: GCSState.mavlinkConnected ? 0 : 2 }
                Text {
                    text: GCSState.mavlinkConnected ? "LINK" : "NO LINK"
                    color: GCSState.mavlinkConnected ? Theme.statusOk : Theme.statusCrit
                    font.pixelSize: Theme.fontSectionLabel
                    font.weight: Font.SemiBold
                    font.letterSpacing: 0.5
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border; opacity: 0.6 }

        // ── GPS ───────────────────────────────────────────────────────────────
        Item {
            Layout.preferredWidth: 150
            Layout.fillHeight: true

            RowLayout {
                anchors { fill: parent; leftMargin: 10 }
                spacing: 6
                StatusDot {
                    level: GCSState.gpsSats >= 6 ? 0 : (GCSState.gpsSats > 0 ? 1 : 2)
                }
                Text {
                    text: "GPS " + GCSState.gpsSats
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSectionLabel
                    font.weight: Font.Medium
                    font.family: "monospace"
                }
                Text {
                    text: GCSState.gpsFixType
                    color: GCSState.gpsFixType === "NO-FIX" ? Theme.statusCrit : Theme.statusOk
                    font.pixelSize: Theme.fontSectionLabel
                    font.weight: Font.Medium
                    font.letterSpacing: 0.4
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border; opacity: 0.6 }

        // ── BATTERY ───────────────────────────────────────────────────────────
        Item {
            Layout.preferredWidth: 180
            Layout.fillHeight: true

            RowLayout {
                anchors { fill: parent; leftMargin: 10 }
                spacing: 6

                Text {
                    text: "BAT"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSectionLabel
                    font.weight: Font.Medium
                }
                Text {
                    text: GCSState.batteryVoltage.toFixed(1) + "V"
                    color: GCSState.batteryPercent < 20 ? Theme.statusCrit
                         : GCSState.batteryPercent < 40 ? Theme.statusWarn
                         : Theme.textPrimary
                    font.pixelSize: Theme.fontSectionLabel
                    font.weight: Font.SemiBold
                    font.family: "monospace"
                }

                // 5-segment bar
                Row {
                    spacing: 2
                    Repeater {
                        model: 5
                        Rectangle {
                            width: 14; height: 8; radius: 1
                            color: {
                                if (Math.round(GCSState.batteryPercent / 20) > index)
                                    return GCSState.batteryPercent < 20 ? Theme.statusCrit
                                         : GCSState.batteryPercent < 40 ? Theme.statusWarn
                                         : Theme.statusOk
                                return Theme.bgElevated
                            }
                            border.color: Theme.border; border.width: 1
                            Behavior on color { ColorAnimation { duration: 300 } }
                        }
                    }
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border; opacity: 0.6 }

        // ── ALTITUDE ─────────────────────────────────────────────────────────
        Item {
            Layout.preferredWidth: 110
            Layout.fillHeight: true

            RowLayout {
                anchors { fill: parent; leftMargin: 10 }
                spacing: 5
                Text {
                    text: "ALT"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSectionLabel
                    font.weight: Font.Medium
                }
                Text {
                    text: GCSState.altitude.toFixed(0).padStart(3, '0') + "m"
                    color: Theme.accentBlue
                    font.pixelSize: Theme.fontSectionLabel
                    font.weight: Font.SemiBold
                    font.family: "monospace"
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border; opacity: 0.6 }

        Item { Layout.fillWidth: true }

        // ── CLOCK ─────────────────────────────────────────────────────────────
        Item {
            Layout.preferredWidth: 76
            Layout.fillHeight: true

            Text {
                id: clockText
                anchors.centerIn: parent
                text: Qt.formatTime(new Date(), "hh:mm")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSectionLabel
                font.weight: Font.Medium
                font.family: "monospace"
                font.letterSpacing: 1.0
            }

            Timer {
                interval: 30000; running: true; repeat: true
                onTriggered: clockText.text = Qt.formatTime(new Date(), "hh:mm")
            }
        }
    }

    // Tap anywhere to open quick panel
    MouseArea {
        anchors.fill: parent
        onClicked: quickPanelToggled()
    }
}
