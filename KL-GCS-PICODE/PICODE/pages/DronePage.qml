import QtQuick
import QtQuick.Layouts
import PICODE

Rectangle {
    color: Theme.bgPrimary

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── LEFT: Telemetry cards ─────────────────────────────────────── 520px ──
        Rectangle {
            Layout.preferredWidth: 520
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
                        text: "TELEMETRY"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // 2×3 DataCard grid
                GridLayout {
                    columns: 2
                    Layout.fillWidth: true
                    Layout.margins: 10
                    rowSpacing: 8; columnSpacing: 8

                    DataCard { label: "ALT";   value: GCSState.altitude.toFixed(1);      unit: "m";    Layout.fillWidth: true; height: 86 }
                    DataCard { label: "SPEED"; value: GCSState.groundSpeed.toFixed(1);   unit: "m/s";  Layout.fillWidth: true; height: 86 }
                    DataCard { label: "HDG";   value: GCSState.heading.toFixed(0);       unit: "°";    Layout.fillWidth: true; height: 86 }
                    DataCard { label: "VSPD";  value: (GCSState.verticalSpeed >= 0 ? "+" : "") + GCSState.verticalSpeed.toFixed(1); unit: "m/s"; Layout.fillWidth: true; height: 86 }
                    DataCard { label: "SAT";   value: GCSState.gpsSats.toString();       unit: "sat";  Layout.fillWidth: true; height: 86 }
                    DataCard { label: "HDOP";  value: GCSState.hdop.toFixed(1);          unit: "HDOP"; Layout.fillWidth: true; height: 86 }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Battery bar
                Item {
                    Layout.fillWidth: true; height: 30
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "BATTERY"
                        color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.margins: 10; spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: GCSState.batteryVoltage.toFixed(1) + "V"
                            color: GCSState.batteryPercent < 20 ? Theme.statusCrit
                                 : GCSState.batteryPercent < 40 ? Theme.statusWarn : Theme.textPrimary
                            font.pixelSize: Theme.fontValueMedium; font.weight: Font.Bold; font.family: "monospace"
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: GCSState.batteryPercent + "%"
                            color: Theme.textSecondary; font.pixelSize: Theme.fontValueSmall; font.weight: Font.SemiBold; font.family: "monospace"
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; height: 10
                        color: Theme.bgElevated; border.color: Theme.border; border.width: 1; radius: 2

                        Rectangle {
                            width: parent.width * Math.max(0, Math.min(1, GCSState.batteryPercent / 100))
                            height: parent.height; radius: 2
                            color: GCSState.batteryPercent < 20 ? Theme.statusCrit
                                 : GCSState.batteryPercent < 40 ? Theme.statusWarn : Theme.statusOk
                            Behavior on width { NumberAnimation { duration: 300 } }
                            Behavior on color { ColorAnimation  { duration: 300 } }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

        // ── RIGHT: Controls ───────────────────────────────────────────── fill ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgSecondary
            border.color: Theme.border; border.width: 1

            ColumnLayout {
                anchors { fill: parent; margins: 0 }
                spacing: 0

                // Flight mode header
                Item {
                    Layout.fillWidth: true; height: 30
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentYellow }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "FLIGHT MODE"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                    // Active mode badge
                    Rectangle {
                        anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                        width: modeLabel.width + 12; height: 20; radius: 3
                        color: Qt.rgba(0.89, 0.82, 0.29, 0.15)
                        border.color: Theme.accentYellow; border.width: 1
                        Text {
                            id: modeLabel
                            anchors.centerIn: parent
                            text: GCSState.flightMode
                            color: Theme.accentYellow; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.5
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Mode override indicators
                RowLayout {
                    Layout.fillWidth: true; Layout.leftMargin: 10; Layout.rightMargin: 10; Layout.topMargin: 6
                    spacing: 6
                    visible: GCSState.sw1Rc || GCSState.sw1Rth

                    Rectangle {
                        visible: GCSState.sw1Rc
                        width: rcLabel.width + 14; height: 22; radius: 3
                        color: Qt.rgba(0.5, 0.84, 1.0, 0.1)
                        border.color: Theme.accentBlue; border.width: 1
                        Text { id: rcLabel; anchors.centerIn: parent; text: "RC: GUIDED"; color: Theme.accentBlue; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.4 }
                    }
                    Rectangle {
                        visible: GCSState.sw1Rth
                        width: rthLabel.width + 14; height: 22; radius: 3
                        color: Qt.rgba(1, 0.7, 0.28, 0.1)
                        border.color: Theme.accentYellow; border.width: 1
                        Text { id: rthLabel; anchors.centerIn: parent; text: "RTH ACTIVE"; color: Theme.accentYellow; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.4 }
                    }
                    Item { Layout.fillWidth: true }
                }

                GridLayout {
                    columns: 3
                    Layout.fillWidth: true; Layout.margins: 10
                    rowSpacing: 6; columnSpacing: 6

                    Repeater {
                        model: ["STABILIZE", "LOITER", "AUTO", "ALT HOLD", "RTL", "GUIDED"]
                        Rectangle {
                            id: modeBtn
                            Layout.fillWidth: true; height: 52; radius: 4
                            property bool isActive: GCSState.flightMode.toUpperCase() === modelData

                            color: isActive ? Qt.rgba(0.89, 0.82, 0.29, 0.12) : Theme.bgElevated
                            border.color: isActive ? Theme.accentYellow : Theme.border
                            border.width: isActive ? 2 : 1

                            Behavior on color        { ColorAnimation  { duration: 120 } }
                            Behavior on border.color { ColorAnimation  { duration: 120 } }
                            scale: modeMa.pressed ? 0.96 : 1.0
                            Behavior on scale        { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }

                            // Left accent bar when active
                            Rectangle {
                                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                                width: 3; radius: 1.5
                                color: Theme.accentYellow
                                visible: isActive
                            }

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: isActive ? Theme.accentYellow : Theme.textSecondary
                                font.pixelSize: Theme.fontButton; font.weight: isActive ? Font.Bold : Font.SemiBold; font.letterSpacing: 0.4
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }

                            MouseArea {
                                id: modeMa
                                anchors.fill: parent
                                onClicked: GCSState.sendFlightMode(modelData)
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // ARM / DISARM section header
                Item {
                    Layout.fillWidth: true; height: 30
                    Rectangle {
                        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                        width: 3
                        color: GCSState.droneArmed ? Theme.statusCrit : Theme.border
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "ARM CONTROL"
                        color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // ARM button with outer glow pulse when armed
                Item {
                    Layout.fillWidth: true; height: 84
                    Layout.margins: 10

                    // Outer glow pulse
                    Rectangle {
                        anchors { fill: parent; margins: -6 }
                        radius: 8; color: "transparent"
                        border.color: Theme.statusCrit; border.width: 2
                        visible: GCSState.droneArmed

                        SequentialAnimation on opacity {
                            running: GCSState.droneArmed
                            loops:   Animation.Infinite
                            NumberAnimation { to: 0.6; duration: 900; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 0.0; duration: 900; easing.type: Easing.InOutSine }
                            onStopped: parent.opacity = 0
                        }
                    }

                    Rectangle {
                        id: armBtn
                        anchors.fill: parent; radius: 5

                        color: {
                            if (!GCSState.keyUnlocked) return Theme.bgElevated
                            return GCSState.droneArmed ? "#2e0c0c" : Theme.bgElevated
                        }
                        border.color: {
                            if (!GCSState.keyUnlocked) return Theme.border
                            return GCSState.droneArmed ? Theme.statusCrit : Theme.border
                        }
                        border.width: GCSState.droneArmed ? 2 : 1

                        Behavior on color        { ColorAnimation  { duration: 200 } }
                        Behavior on border.color { ColorAnimation  { duration: 200 } }

                        // HW ARM badge
                        Rectangle {
                            anchors { top: parent.top; right: parent.right; margins: 6 }
                            visible: GCSState.sw1Arm
                            width: hwArmLabel.width + 10; height: 18; radius: 3
                            color: Qt.rgba(1, 0.36, 0.36, 0.15)
                            border.color: Theme.statusCrit; border.width: 1
                            Text { id: hwArmLabel; anchors.centerIn: parent; text: "HW ARM"; color: Theme.statusCrit; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 0.3 }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent; spacing: 3
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: !GCSState.keyUnlocked ? "🔒  KEY REQUIRED"
                                    : GCSState.droneArmed   ? "DISARM" : "ARM"
                                color: !GCSState.keyUnlocked ? Theme.textDisabled
                                     : GCSState.droneArmed   ? Theme.statusCrit : Theme.textPrimary
                                font.pixelSize: 18; font.weight: Font.Bold; font.letterSpacing: 1.2
                                Behavior on color { ColorAnimation { duration: 200 } }
                            }
                            Text {
                                visible: GCSState.keyUnlocked
                                Layout.alignment: Qt.AlignHCenter
                                text: GCSState.droneArmed ? "MOTORS ENABLED" : "MOTORS SAFE"
                                color: GCSState.droneArmed ? Qt.rgba(1,0.36,0.36,0.7) : Theme.textDisabled
                                font.pixelSize: 10; font.letterSpacing: 0.6
                            }
                        }

                        scale: armMa.pressed ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 80 } }

                        MouseArea {
                            id: armMa
                            anchors.fill: parent
                            enabled: GCSState.keyUnlocked
                            onClicked: armConfirm.open()
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // System messages
                Item {
                    Layout.fillWidth: true; height: 26
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "SYSTEM LOG"
                        color: Theme.textDisabled; font.pixelSize: 10; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 10; spacing: 4

                    Repeater {
                        model: GCSState.statusMessages
                        Text {
                            text: "> " + modelData
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSectionLabel; font.family: "monospace"
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    ConfirmOverlay {
        id: armConfirm
        title: GCSState.droneArmed ? "DISARM DRONE" : "ARM DRONE"
        body: GCSState.droneArmed
            ? "Disarming will cut motor power immediately."
            : "Arming will enable motors. Ensure the area is clear."
        confirmLabel: GCSState.droneArmed ? "DISARM" : "ARM"
        destructive: true
        onConfirmed: GCSState.sendArmDisarm(!GCSState.droneArmed)
    }
}
