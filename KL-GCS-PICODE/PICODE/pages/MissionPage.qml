import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import PICODE

Rectangle {
    color: Theme.bgPrimary

    property int selectedWpIndex: -1

    readonly property var defaultCenter: QtPositioning.coordinate(52.3731, 4.8965)

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── LEFT: Editable map  640px ─────────────────────────────────────────
        Item {
            Layout.preferredWidth: 640
            Layout.fillHeight: true

            Map {
                id: missionMap
                anchors.fill: parent
                plugin: Plugin {
                    name: "osm"
                    PluginParameter { name: "osm.useragent"; value: "KL-GCS/1.0" }
                }
                center: defaultCenter
                zoomLevel: 14
                copyrightsVisible: false

                // Tap-to-add waypoint
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: function(mouse) {
                        var coord = missionMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                        var wps = []
                        for (var i = 0; i < GCSState.waypoints.length; i++)
                            wps.push(GCSState.waypoints[i])
                        var wpType = wps.length === 0 ? "TAKEOFF"
                                   : "WAYPOINT"
                        wps.push({ lat: coord.latitude, lon: coord.longitude, alt: 20, type: wpType })
                        GCSState.waypoints = wps
                        selectedWpIndex = wps.length - 1
                    }
                }

                // Drone marker (read-only)
                MapQuickItem {
                    visible: GCSState.droneLat !== 0.0 || GCSState.droneLon !== 0.0
                    coordinate: QtPositioning.coordinate(GCSState.droneLat, GCSState.droneLon)
                    anchorPoint.x: 8; anchorPoint.y: 8
                    sourceItem: Rectangle {
                        width: 16; height: 16; radius: 8
                        color: Theme.accentYellow; border.color: Theme.bgPrimary; border.width: 2
                    }
                }

                // WP path line
                MapPolyline {
                    visible: GCSState.waypoints.length >= 2
                    line.color: Theme.accentBlue; line.width: 2
                    path: {
                        var pts = []
                        for (var i = 0; i < GCSState.waypoints.length; i++)
                            pts.push(QtPositioning.coordinate(GCSState.waypoints[i].lat || 0, GCSState.waypoints[i].lon || 0))
                        return pts
                    }
                }

                // WP markers
                MapItemView {
                    model: GCSState.waypoints
                    delegate: MapQuickItem {
                        coordinate: QtPositioning.coordinate(modelData.lat || 0, modelData.lon || 0)
                        anchorPoint.x: 14; anchorPoint.y: 14
                        sourceItem: Rectangle {
                            width: 28; height: 28; radius: 14
                            color: selectedWpIndex === index ? Theme.accentYellow : Theme.bgSecondary
                            border.color: selectedWpIndex === index ? Theme.accentYellow : Theme.accentBlue
                            border.width: 2
                            Text {
                                anchors.centerIn: parent
                                text: index + 1
                                color: selectedWpIndex === index ? Theme.bgPrimary : Theme.accentBlue
                                font.pixelSize: 11; font.weight: Font.Bold
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: selectedWpIndex = index
                            }
                        }
                    }
                }

                PinchHandler { }
                WheelHandler { acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad }
            }

            // Hint overlay
            Rectangle {
                anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 8 }
                color: Qt.rgba(0, 0, 0, 0.55); radius: 4
                width: hintText.width + 16; height: hintText.height + 8
                Text {
                    id: hintText
                    anchors.centerIn: parent
                    text: "TAP MAP TO ADD WAYPOINT  ·  TAP MARKER TO SELECT"
                    color: Theme.textDisabled; font.pixelSize: 10
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

        // ── RIGHT: Mission editor  384px ─────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // WP list header
            Item {
                Layout.fillWidth: true; height: 32
                Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentYellow }
                RowLayout {
                    anchors { fill: parent; leftMargin: 11; rightMargin: 10 }
                    Text {
                        text: "MISSION EDITOR"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                        Layout.fillWidth: true
                    }
                    Text {
                        text: GCSState.waypoints.length + " WP"
                        color: GCSState.waypoints.length > 0 ? Theme.accentYellow : Theme.textDisabled
                        font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold; font.family: "monospace"
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            // WP list
            ListView {
                id: wpEditor
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: GCSState.waypoints
                spacing: 0

                delegate: Rectangle {
                    width: wpEditor.width; height: 64
                    color: selectedWpIndex === index ? Qt.rgba(0.89, 0.82, 0.29, 0.06) : (index % 2 === 0 ? Theme.bgPrimary : Theme.bgSecondary)
                    border.color: selectedWpIndex === index ? Theme.accentYellow : Theme.border
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 100 } }

                    // Left accent bar when selected
                    Rectangle {
                        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                        width: 3; color: Theme.accentYellow
                        visible: selectedWpIndex === index
                    }

                    ColumnLayout {
                        anchors { fill: parent; leftMargin: selectedWpIndex === index ? 11 : 8; rightMargin: 8; topMargin: 8; bottomMargin: 8 }
                        spacing: 3

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Text {
                                text: "WP" + (index + 1)
                                color: Theme.accentYellow; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold; font.family: "monospace"
                                Layout.preferredWidth: 36
                            }
                            Text {
                                text: modelData.type ? modelData.type : "WAYPOINT"
                                color: Theme.textPrimary; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium
                                Layout.fillWidth: true
                            }
                            Text {
                                text: modelData.alt !== undefined ? modelData.alt + " m" : ""
                                color: Theme.accentBlue; font.pixelSize: Theme.fontSectionLabel; font.family: "monospace"
                            }
                        }
                        Text {
                            text: (modelData.lat !== undefined ? modelData.lat.toFixed(5) : "--")
                                  + ", " + (modelData.lon !== undefined ? modelData.lon.toFixed(5) : "--")
                            color: Theme.textDisabled; font.pixelSize: 10; font.family: "monospace"
                        }
                    }

                    MouseArea { anchors.fill: parent; onClicked: selectedWpIndex = index }
                }
            }

            // ADD / DEL buttons
            Rectangle {
                Layout.fillWidth: true; height: 56
                color: Theme.bgSecondary

                RowLayout {
                    anchors { fill: parent; margins: 8 }
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true; height: 40; radius: 4
                        color: addMa.pressed ? Qt.rgba(0.89, 0.82, 0.29, 0.10) : Theme.bgElevated
                        border.color: addMa.pressed ? Theme.accentYellow : Theme.border; border.width: 1
                        Behavior on color { ColorAnimation { duration: 80 } }
                        scale: addMa.pressed ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 70 } }
                        Text { anchors.centerIn: parent; text: "+ ADD WP"; color: Theme.accentYellow; font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold }
                        MouseArea {
                            id: addMa; anchors.fill: parent
                            onClicked: {
                                var wps = []
                                for (var i = 0; i < GCSState.waypoints.length; i++)
                                    wps.push(GCSState.waypoints[i])
                                wps.push({ lat: missionMap.center.latitude, lon: missionMap.center.longitude, alt: 20, type: "WAYPOINT" })
                                GCSState.waypoints = wps
                                selectedWpIndex = wps.length - 1
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; height: 40; radius: 4
                        property bool canDel: selectedWpIndex >= 0
                        color: canDel && delMa.pressed ? Qt.rgba(1, 0.36, 0.36, 0.08) : Theme.bgElevated
                        border.color: canDel ? (delMa.pressed ? Theme.statusCrit : Theme.border) : Theme.bgElevated; border.width: 1
                        Behavior on color { ColorAnimation { duration: 80 } }
                        scale: delMa.pressed && canDel ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 70 } }
                        Text {
                            anchors.centerIn: parent; text: "– DEL WP"
                            color: parent.canDel ? Theme.statusCrit : Theme.textDisabled
                            font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold
                        }
                        MouseArea {
                            id: delMa; anchors.fill: parent
                            enabled: parent.canDel
                            onClicked: {
                                var wps = []
                                for (var i = 0; i < GCSState.waypoints.length; i++)
                                    if (i !== selectedWpIndex) wps.push(GCSState.waypoints[i])
                                GCSState.waypoints = wps
                                selectedWpIndex = Math.min(selectedWpIndex, wps.length - 1)
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            // UPLOAD / DOWNLOAD / CLEAR
            Repeater {
                model: [
                    { label: "UPLOAD TO DRONE",    action: "upload",   accent: Theme.accentBlue  },
                    { label: "DOWNLOAD FROM DRONE", action: "download", accent: Theme.accentBlue  },
                    { label: "CLEAR MISSION",       action: "clear",    accent: Theme.statusCrit  }
                ]
                Rectangle {
                    id: missionActBtn
                    Layout.fillWidth: true; height: 60
                    property color accentColor: modelData.accent
                    color: missionActMa.pressed ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.08) : Theme.bgElevated
                    border.color: missionActMa.pressed ? accentColor : Theme.border; border.width: 1
                    Behavior on color { ColorAnimation { duration: 80 } }
                    Behavior on border.color { ColorAnimation { duration: 80 } }

                    // top accent line
                    Rectangle {
                        anchors { top: parent.top; left: parent.left; right: parent.right }
                        height: 2
                        color: missionActBtn.accentColor
                        opacity: missionActMa.pressed ? 0.8 : 0.2
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }

                    scale: missionActMa.pressed ? 0.98 : 1.0
                    Behavior on scale { NumberAnimation { duration: 70 } }

                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: missionActMa.pressed ? missionActBtn.accentColor : Theme.textSecondary
                        font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold; font.letterSpacing: 0.4
                        Behavior on color { ColorAnimation { duration: 80 } }
                    }
                    MouseArea {
                        id: missionActMa; anchors.fill: parent
                        onClicked: {
                            if (modelData.action === "upload")   uploadConfirm.open()
                            if (modelData.action === "download") GCSState.sendMissionDownload()
                            if (modelData.action === "clear")    clearConfirm.open()
                        }
                    }
                }
            }
        }
    }

    // ── Confirm overlays ──────────────────────────────────────────────────────
    ConfirmOverlay {
        id: uploadConfirm
        title: "UPLOAD MISSION TO DRONE"
        body: "This will replace the current mission on the drone with the mission shown here."
        confirmLabel: "UPLOAD"
        destructive: false
        onConfirmed: GCSState.sendMissionUpload()
    }

    ConfirmOverlay {
        id: clearConfirm
        title: "CLEAR MISSION"
        body: "All waypoints will be deleted from the editor. The drone's stored mission is not affected until you upload."
        confirmLabel: "CLEAR"
        destructive: true
        onConfirmed: { GCSState.waypoints = []; selectedWpIndex = -1 }
    }
}
