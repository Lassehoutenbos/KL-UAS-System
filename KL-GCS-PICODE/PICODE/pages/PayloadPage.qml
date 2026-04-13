import QtQuick
import QtQuick.Layouts
import PICODE

Rectangle {
    color: Theme.bgPrimary

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── LEFT: Payload status ────────────────────────────────────── 420px ──
        Rectangle {
            Layout.preferredWidth: 420
            Layout.fillHeight: true
            color: Theme.bgSecondary
            border.color: Theme.border; border.width: 1

            ColumnLayout {
                anchors { fill: parent; margins: 0 }
                spacing: 0

                // Section header
                Item {
                    Layout.fillWidth: true; height: 30
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentYellow }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "PAYLOAD BAYS"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // PAY1
                Item {
                    Layout.fillWidth: true; Layout.preferredHeight: 180; Layout.margins: 12

                    Rectangle {
                        anchors.fill: parent; radius: 6
                        color: GCSState.sw3Pay1Arm ? Qt.rgba(1, 0.36, 0.36, 0.06) : Theme.bgElevated
                        border.color: GCSState.sw3Pay1Arm ? Theme.statusCrit : Theme.border
                        border.width: GCSState.sw3Pay1Arm ? 2 : 1
                        Behavior on color { ColorAnimation { duration: 200 } }
                        Behavior on border.color { ColorAnimation { duration: 200 } }

                        ColumnLayout {
                            anchors { fill: parent; margins: 14 }
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "PAY 1"; color: Theme.textPrimary; font.pixelSize: 16; font.weight: Font.Bold; font.letterSpacing: 0.6 }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: armBadge1.width + 12; height: 22; radius: 3
                                    color: GCSState.sw3Pay1Arm ? Qt.rgba(1, 0.36, 0.36, 0.15) : Qt.rgba(1, 1, 1, 0.05)
                                    border.color: GCSState.sw3Pay1Arm ? Theme.statusCrit : Theme.border; border.width: 1
                                    Text {
                                        id: armBadge1; anchors.centerIn: parent
                                        text: GCSState.sw3Pay1Arm ? "ARMED" : "SAFE"
                                        color: GCSState.sw3Pay1Arm ? Theme.statusCrit : Theme.statusOk
                                        font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.5
                                    }
                                }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                            RowLayout {
                                Layout.fillWidth: true; spacing: 16
                                ColumnLayout {
                                    spacing: 2
                                    Text { text: "ARM SWITCH"; color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.Medium; font.letterSpacing: 0.4 }
                                    RowLayout {
                                        spacing: 6
                                        StatusDot { level: GCSState.sw3Pay1Arm ? 2 : 0 }
                                        Text {
                                            text: GCSState.sw3Pay1Arm ? "ON" : "OFF"
                                            color: GCSState.sw3Pay1Arm ? Theme.statusCrit : Theme.textSecondary
                                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                                        }
                                    }
                                }
                                ColumnLayout {
                                    spacing: 2
                                    Text { text: "FIRE BUTTON"; color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.Medium; font.letterSpacing: 0.4 }
                                    RowLayout {
                                        spacing: 6
                                        StatusDot { level: GCSState.sw3Pay1Fire ? 2 : -1 }
                                        Text {
                                            text: GCSState.sw3Pay1Fire ? "FIRING" : "IDLE"
                                            color: GCSState.sw3Pay1Fire ? Theme.statusCrit : Theme.textSecondary
                                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                                        }
                                    }
                                }
                            }

                            // Fire indicator bar
                            Rectangle {
                                Layout.fillWidth: true; height: 8; radius: 2
                                color: Theme.bgSecondary; border.color: Theme.border; border.width: 1

                                Rectangle {
                                    width: GCSState.sw3Pay1Fire ? parent.width : 0
                                    height: parent.height; radius: 2
                                    color: Theme.statusCrit
                                    Behavior on width { NumberAnimation { duration: 100 } }
                                }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }
                }

                // PAY2
                Item {
                    Layout.fillWidth: true; Layout.preferredHeight: 180; Layout.margins: 12; Layout.topMargin: 0

                    Rectangle {
                        anchors.fill: parent; radius: 6
                        color: GCSState.sw3Pay2Arm ? Qt.rgba(1, 0.36, 0.36, 0.06) : Theme.bgElevated
                        border.color: GCSState.sw3Pay2Arm ? Theme.statusCrit : Theme.border
                        border.width: GCSState.sw3Pay2Arm ? 2 : 1
                        Behavior on color { ColorAnimation { duration: 200 } }
                        Behavior on border.color { ColorAnimation { duration: 200 } }

                        ColumnLayout {
                            anchors { fill: parent; margins: 14 }
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "PAY 2"; color: Theme.textPrimary; font.pixelSize: 16; font.weight: Font.Bold; font.letterSpacing: 0.6 }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: armBadge2.width + 12; height: 22; radius: 3
                                    color: GCSState.sw3Pay2Arm ? Qt.rgba(1, 0.36, 0.36, 0.15) : Qt.rgba(1, 1, 1, 0.05)
                                    border.color: GCSState.sw3Pay2Arm ? Theme.statusCrit : Theme.border; border.width: 1
                                    Text {
                                        id: armBadge2; anchors.centerIn: parent
                                        text: GCSState.sw3Pay2Arm ? "ARMED" : "SAFE"
                                        color: GCSState.sw3Pay2Arm ? Theme.statusCrit : Theme.statusOk
                                        font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.5
                                    }
                                }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                            RowLayout {
                                Layout.fillWidth: true; spacing: 16
                                ColumnLayout {
                                    spacing: 2
                                    Text { text: "ARM SWITCH"; color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.Medium; font.letterSpacing: 0.4 }
                                    RowLayout {
                                        spacing: 6
                                        StatusDot { level: GCSState.sw3Pay2Arm ? 2 : 0 }
                                        Text {
                                            text: GCSState.sw3Pay2Arm ? "ON" : "OFF"
                                            color: GCSState.sw3Pay2Arm ? Theme.statusCrit : Theme.textSecondary
                                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                                        }
                                    }
                                }
                                ColumnLayout {
                                    spacing: 2
                                    Text { text: "FIRE BUTTON"; color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.Medium; font.letterSpacing: 0.4 }
                                    RowLayout {
                                        spacing: 6
                                        StatusDot { level: GCSState.sw3Pay2Fire ? 2 : -1 }
                                        Text {
                                            text: GCSState.sw3Pay2Fire ? "FIRING" : "IDLE"
                                            color: GCSState.sw3Pay2Fire ? Theme.statusCrit : Theme.textSecondary
                                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                                        }
                                    }
                                }
                            }

                            // Fire indicator bar
                            Rectangle {
                                Layout.fillWidth: true; height: 8; radius: 2
                                color: Theme.bgSecondary; border.color: Theme.border; border.width: 1

                                Rectangle {
                                    width: GCSState.sw3Pay2Fire ? parent.width : 0
                                    height: parent.height; radius: 2
                                    color: Theme.statusCrit
                                    Behavior on width { NumberAnimation { duration: 100 } }
                                }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

        // ── RIGHT: Safety status & info ─────────────────────────────────── fill ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgSecondary
            border.color: Theme.border; border.width: 1

            ColumnLayout {
                anchors { fill: parent; margins: 0 }
                spacing: 0

                // Section header
                Item {
                    Layout.fillWidth: true; height: 30
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "SAFETY"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.margins: 14; spacing: 10

                    // Key switch status
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        StatusDot { level: GCSState.keyUnlocked ? 0 : 1 }
                        Text { text: "KEY SWITCH"; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 100; font.letterSpacing: 0.4 }
                        Text {
                            text: GCSState.keyUnlocked ? "UNLOCKED" : "LOCKED"
                            color: GCSState.keyUnlocked ? Theme.statusOk : Theme.statusWarn
                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                        }
                    }

                    // Master arm status
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        StatusDot { level: GCSState.sw1Arm ? 2 : 0 }
                        Text { text: "MASTER ARM"; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 100; font.letterSpacing: 0.4 }
                        Text {
                            text: GCSState.sw1Arm ? "ARMED" : "SAFE"
                            color: GCSState.sw1Arm ? Theme.statusCrit : Theme.statusOk
                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                        }
                    }

                    // Drone armed status
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        StatusDot { level: GCSState.droneArmed ? 2 : 0 }
                        Text { text: "DRONE"; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium; Layout.preferredWidth: 100; font.letterSpacing: 0.4 }
                        Text {
                            text: GCSState.droneArmed ? "ARMED" : "DISARMED"
                            color: GCSState.droneArmed ? Theme.statusCrit : Theme.statusOk
                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Info section
                Item {
                    Layout.fillWidth: true; height: 26
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "PAYLOAD INFO"
                        color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.margins: 14; spacing: 6

                    Text {
                        text: "Payload bays are stub-only. No commands are sent."
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel
                        wrapMode: Text.Wrap; Layout.fillWidth: true
                    }
                    Text {
                        text: "LED feedback active on WS2811 chain:"
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: [
                            { lbl: "DISARMED", col: Theme.textDisabled, desc: "LED off" },
                            { lbl: "ARMED",    col: Theme.statusWarn,   desc: "Blink orange" },
                            { lbl: "FIRING",   col: Theme.statusCrit,   desc: "Solid red" }
                        ]
                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Rectangle { width: 8; height: 8; radius: 4; color: modelData.col }
                            Text { text: modelData.lbl; color: modelData.col; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold; font.letterSpacing: 0.4; Layout.preferredWidth: 70 }
                            Text { text: modelData.desc; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
