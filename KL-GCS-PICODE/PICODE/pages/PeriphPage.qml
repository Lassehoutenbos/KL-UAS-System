import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PICODE

Rectangle {
    color: Theme.bgPrimary

    property int  selectedIndex:    -1
    property int  tftActiveAddress: -1  // address currently shown on TFT detail screen

    property var selectedDevice: selectedIndex >= 0 && selectedIndex < GCSState.peripherals.length
                                 ? GCSState.peripherals[selectedIndex] : null

    readonly property int onlineCount: {
        var n = 0
        for (var i = 0; i < GCSState.peripherals.length; i++)
            if (GCSState.peripherals[i].online) n++
        return n
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── LEFT SIDEBAR  260px ───────────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 260
            Layout.fillHeight: true
            color: Theme.bgSecondary
            border.color: Theme.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header
                Item {
                    Layout.fillWidth: true; height: 32
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentYellow }
                    RowLayout {
                        anchors { fill: parent; leftMargin: 11; rightMargin: 10 }
                        Text { text: "DEVICES"; color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8; Layout.fillWidth: true }
                        Text { text: onlineCount + "/" + GCSState.peripherals.length; color: Theme.accentBlue; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Device list
                ListView {
                    id: deviceList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    topMargin: 6; bottomMargin: 6; leftMargin: 8; rightMargin: 8
                    clip: true
                    model: GCSState.peripherals
                    spacing: 2

                    delegate: Rectangle {
                        width: deviceList.width - 16
                        height: Theme.minTouchSize
                        radius: 4
                        color: selectedIndex === index ? Theme.bgElevated : "transparent"
                        border.color: selectedIndex === index ? Theme.accentYellow : "transparent"
                        border.width: 1

                        // Left accent when selected
                        Rectangle {
                            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                            width: 3; radius: 1.5; color: Theme.accentYellow
                            visible: selectedIndex === index
                        }

                        RowLayout {
                            anchors { fill: parent; leftMargin: selectedIndex === index ? 10 : 6; rightMargin: 6 }
                            spacing: 6

                            StatusDot { level: modelData.online ? 0 : 2 }

                            Text {
                                text: "0x" + modelData.address.toString(16).toUpperCase().padStart(2, '0')
                                color: Theme.textDisabled
                                font.pixelSize: Theme.fontSectionLabel
                                font.family: "monospace"
                            }

                            Text {
                                text: modelData.name
                                color: modelData.online ? Theme.textPrimary : Theme.textDisabled
                                font.pixelSize: Theme.fontSectionLabel
                                font.weight: Font.Medium
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            // [TFT] tag — shows which device is on TFT detail view
                            Rectangle {
                                visible: tftActiveAddress === modelData.address
                                width: 32; height: 18; radius: 3
                                color: Qt.rgba(0.22, 0.55, 1.0, 0.12)
                                border.color: Theme.accentBlue; border.width: 1
                                Text { anchors.centerIn: parent; text: "TFT"; color: Theme.accentBlue; font.pixelSize: 9; font.weight: Font.Bold }
                            }
                        }

                        MouseArea { anchors.fill: parent; onClicked: selectedIndex = index }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // TFT control buttons
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    spacing: 6

                    // VIEW ON TFT
                    Rectangle {
                        Layout.fillWidth: true
                        height: Theme.minTouchSize
                        radius: 4
                        property bool canUse: selectedDevice !== null && selectedDevice.online
                        color: canUse ? Qt.rgba(0.89, 0.82, 0.29, 0.10) : Theme.bgSecondary
                        border.color: canUse ? Theme.accentYellow : Theme.border
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "VIEW ON TFT"
                            color: parent.canUse ? Theme.accentYellow : Theme.textDisabled
                            font.pixelSize: Theme.fontButton
                            font.weight: Font.SemiBold
                        }
                        MouseArea {
                            anchors.fill: parent
                            enabled: parent.canUse
                            onClicked: {
                                var addr = selectedDevice.address
                                GCSState.sendTftPeriphDetail(addr)
                                tftActiveAddress = addr
                            }
                        }
                    }

                    // TFT: OVERVIEW
                    Rectangle {
                        Layout.fillWidth: true
                        height: Theme.minTouchSize
                        radius: 4
                        color: Theme.bgElevated
                        border.color: Theme.border; border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "TFT: OVERVIEW"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontButton
                            font.weight: Font.SemiBold
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { GCSState.sendTftScreen(5); tftActiveAddress = -1 }
                        }
                    }
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

        // ── RIGHT DETAIL PANEL ────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Empty state
            ColumnLayout {
                anchors.centerIn: parent
                visible: selectedDevice === null
                spacing: 8
                Text { text: "NO DEVICE SELECTED"; color: Theme.textDisabled; font.pixelSize: Theme.fontValueMedium; font.weight: Font.SemiBold; Layout.alignment: Qt.AlignHCenter }
                Text { text: "Select a device from the list."; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; Layout.alignment: Qt.AlignHCenter }
            }

            // Device detail
            ColumnLayout {
                anchors { fill: parent; margins: 12 }
                spacing: 8
                visible: selectedDevice !== null

                // Header row
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text {
                        text: selectedDevice ? selectedDevice.name : ""
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontPageTitle; font.weight: Font.Bold; font.letterSpacing: 0.6
                        Layout.fillWidth: true
                    }
                    Rectangle {
                        width: addrLabel.width + 10; height: 22; radius: 3
                        color: Theme.bgElevated; border.color: Theme.border; border.width: 1
                        Text {
                            id: addrLabel
                            anchors.centerIn: parent
                            text: selectedDevice ? "0x" + selectedDevice.address.toString(16).toUpperCase().padStart(2, '0') : ""
                            color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.family: "monospace"
                        }
                    }
                    Rectangle {
                        width: statusLabel.width + 16; height: 22; radius: 3
                        color: (selectedDevice && selectedDevice.online) ? Qt.rgba(0.2, 0.8, 0.4, 0.12) : Qt.rgba(1, 0.36, 0.36, 0.08)
                        border.color: (selectedDevice && selectedDevice.online) ? Theme.statusOk : Theme.statusCrit
                        border.width: 1
                        RowLayout {
                            anchors.centerIn: parent; spacing: 5
                            StatusDot { level: selectedDevice && selectedDevice.online ? 0 : 2 }
                            Text {
                                id: statusLabel
                                text: selectedDevice && selectedDevice.online ? "ONLINE" : "OFFLINE"
                                color: selectedDevice && selectedDevice.online ? Theme.statusOk : Theme.statusCrit
                                font.pixelSize: Theme.fontSectionLabel; font.weight: Font.SemiBold
                            }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Type-specific detail
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: selectedDevice !== null
                    sourceComponent: {
                        if (!selectedDevice) return null
                        if (selectedDevice.name === "SEARCHLIGHT") return searchlightComp
                        if (selectedDevice.name === "PAN-TILT")    return panTiltComp
                        if (selectedDevice.name === "RADAR")        return radarComp
                        return genericComp
                    }
                }

                // Command buttons
                RowLayout {
                    Layout.fillWidth: true; spacing: 6
                    Repeater {
                        model: [
                            { label: "STREAM\nON 100ms", cmd: 0x30 },
                            { label: "GET STATUS",        cmd: 0x20 },
                            { label: "SET PARAM",         cmd: 0xFF },
                            { label: "PING",              cmd: 0x01 }
                        ]
                        Rectangle {
                            id: cmdBtn
                            Layout.fillWidth: true; height: 56; radius: 4
                            property bool enabled_: selectedDevice !== null && selectedDevice.online
                            color: cmdMa.pressed && enabled_ ? Qt.rgba(0.89, 0.82, 0.29, 0.08) : Theme.bgElevated
                            border.color: cmdMa.pressed && enabled_ ? Theme.accentYellow : Theme.border
                            border.width: 1
                            opacity: enabled_ ? 1.0 : 0.38
                            Behavior on color { ColorAnimation { duration: 80 } }
                            Behavior on border.color { ColorAnimation { duration: 80 } }
                            scale: cmdMa.pressed && enabled_ ? 0.96 : 1.0
                            Behavior on scale { NumberAnimation { duration: 70; easing.type: Easing.OutQuad } }
                            Text {
                                anchors.centerIn: parent; text: modelData.label
                                color: Theme.textSecondary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium
                                horizontalAlignment: Text.AlignHCenter
                            }
                            MouseArea {
                                id: cmdMa
                                anchors.fill: parent
                                enabled: parent.enabled_
                                onClicked: GCSState.sendPeriphCmd(selectedDevice.address, modelData.cmd)
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Searchlight detail ────────────────────────────────────────────────────
    Component {
        id: searchlightComp
        ColumnLayout {
            spacing: 10

            Text { text: "BRIGHTNESS"; color: Theme.textSecondary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium }
            SliderRow {
                label: "BRIGHTNESS"
                value: selectedDevice && selectedDevice.brightness !== undefined ? selectedDevice.brightness : 0
                Layout.fillWidth: true
                onSliderMoved: function(v) {
                    if (selectedDevice) {
                        var ba = new ArrayBuffer(1)
                        var dv = new DataView(ba)
                        dv.setUint8(0, v)
                        GCSState.sendPeriphCmd(selectedDevice.address, 0x10)
                    }
                }
            }

            RowLayout { Layout.fillWidth: true; spacing: 24
                RowLayout {
                    spacing: 8
                    Text { text: "TEMPERATURE"; color: Theme.textSecondary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium }
                    StatusDot {
                        level: selectedDevice && selectedDevice.temp !== undefined
                               ? (selectedDevice.temp < 60 ? 0 : (selectedDevice.temp < 80 ? 1 : 2)) : -1
                    }
                    Text {
                        text: selectedDevice && selectedDevice.temp !== undefined ? selectedDevice.temp + "°C" : "--"
                        color: Theme.textPrimary; font.pixelSize: Theme.fontValueSmall; font.weight: Font.SemiBold
                    }
                }
                RowLayout {
                    spacing: 8
                    Text { text: "FAULTS"; color: Theme.textSecondary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium }
                    StatusDot {
                        level: selectedDevice && selectedDevice.faults !== undefined
                               ? (selectedDevice.faults === 0 ? 0 : 2) : -1
                    }
                    Text {
                        text: selectedDevice && selectedDevice.faults !== undefined
                              ? (selectedDevice.faults === 0 ? "NONE" : "0x" + selectedDevice.faults.toString(16).toUpperCase())
                              : "--"
                        color: Theme.textPrimary; font.pixelSize: Theme.fontValueSmall; font.weight: Font.SemiBold
                    }
                }
            }
            Item { Layout.fillHeight: true }
        }
    }

    // ── Pan-Tilt detail ───────────────────────────────────────────────────────
    Component {
        id: panTiltComp
        ColumnLayout {
            spacing: 10
            SliderRow {
                label: "PAN"
                value: selectedDevice && selectedDevice.pan !== undefined ? selectedDevice.pan : 0
                Layout.fillWidth: true
                onSliderMoved: function(v) { if (selectedDevice) GCSState.sendPeriphCmd(selectedDevice.address, 0x11) }
            }
            SliderRow {
                label: "TILT"
                value: selectedDevice && selectedDevice.tilt !== undefined ? selectedDevice.tilt : 0
                Layout.fillWidth: true
                onSliderMoved: function(v) { if (selectedDevice) GCSState.sendPeriphCmd(selectedDevice.address, 0x12) }
            }
            Item { Layout.fillHeight: true }
        }
    }

    // ── Radar detail ──────────────────────────────────────────────────────────
    Component {
        id: radarComp
        ColumnLayout {
            spacing: 10
            RowLayout {
                spacing: 12
                Text { text: "DISTANCE"; color: Theme.textSecondary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium }
                Text {
                    text: selectedDevice && selectedDevice.distance !== undefined
                          ? selectedDevice.distance.toFixed(1) + " m" : "--"
                    color: Theme.accentBlue; font.pixelSize: Theme.fontValueLarge; font.weight: Font.Bold
                }
            }
            Item { Layout.fillHeight: true }
        }
    }

    // ── Generic detail ────────────────────────────────────────────────────────
    Component {
        id: genericComp
        ColumnLayout {
            spacing: 8
            Text {
                text: selectedDevice && selectedDevice.online
                      ? "Device online — no detail view for this device type."
                      : "Device offline — no data available."
                color: Theme.textSecondary; font.pixelSize: Theme.fontSectionLabel
                wrapMode: Text.Wrap; Layout.fillWidth: true
            }
            Item { Layout.fillHeight: true }
        }
    }
}
