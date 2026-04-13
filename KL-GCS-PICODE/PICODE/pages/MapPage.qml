import QtQuick
import QtQuick.Layouts
import QtLocation
import QtPositioning
import PICODE

Rectangle {
    color: Theme.bgPrimary

    // Default centre: Amsterdam (shown when no GPS fix yet)
    readonly property var defaultCenter: QtPositioning.coordinate(52.3731, 4.8965)
    readonly property bool hasDroneFix: GCSState.droneLat !== 0.0 || GCSState.droneLon !== 0.0

    // Total mission distance (crude straight-line sum over waypoints)
    readonly property real missionDistM: {
        var wps = GCSState.waypoints
        var dist = 0.0
        for (var i = 1; i < wps.length; i++) {
            var a = QtPositioning.coordinate(wps[i-1].lat, wps[i-1].lon)
            var b = QtPositioning.coordinate(wps[i].lat,   wps[i].lon)
            dist += a.distanceTo(b)
        }
        return dist
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── LEFT SIDEBAR  220px ───────────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: Theme.bgSecondary
            border.color: Theme.border; border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header
                Item {
                    Layout.fillWidth: true; height: 32
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    RowLayout {
                        anchors { fill: parent; leftMargin: 11; rightMargin: 10 }
                        Text { text: "WAYPOINTS"; color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8; Layout.fillWidth: true }
                        Text { text: GCSState.waypoints.length + " WP"; color: Theme.accentBlue; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold; font.family: "monospace" }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // WP list
                ListView {
                    id: wpListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    topMargin: 6; bottomMargin: 4; leftMargin: 8; rightMargin: 8
                    clip: true
                    model: GCSState.waypoints
                    spacing: 3

                    delegate: Rectangle {
                        width: wpListView.width - 16
                        height: 52; radius: 4
                        color: Theme.bgElevated
                        border.color: Theme.border; border.width: 1

                        // left accent
                        Rectangle {
                            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                            width: 3; radius: 1.5
                            color: Theme.accentYellow; opacity: 0.6
                        }

                        ColumnLayout {
                            anchors { fill: parent; leftMargin: 10; rightMargin: 6; topMargin: 5; bottomMargin: 5 }
                            spacing: 2
                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "WP" + (index + 1)
                                    color: Theme.accentYellow
                                    font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold; font.family: "monospace"
                                    Layout.preferredWidth: 36
                                }
                                Text {
                                    text: modelData.type ? modelData.type : "WAYPOINT"
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium
                                    Layout.fillWidth: true; elide: Text.ElideRight
                                }
                            }
                            Text {
                                text: {
                                    var s = modelData.alt !== undefined ? modelData.alt + "m" : ""
                                    if (index > 0) {
                                        var prev = GCSState.waypoints[index - 1]
                                        var c1 = QtPositioning.coordinate(prev.lat, prev.lon)
                                        var c2 = QtPositioning.coordinate(modelData.lat, modelData.lon)
                                        s += "  +" + Math.round(c1.distanceTo(c2)) + "m"
                                    }
                                    return s
                                }
                                color: Theme.textDisabled
                                font.pixelSize: 10; font.family: "monospace"
                            }
                        }
                    }
                }

                // Distance summary
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                Rectangle {
                    Layout.fillWidth: true; height: 36
                    color: Theme.bgSecondary
                    RowLayout {
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                        Text { text: "TOTAL"; color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: missionDistM >= 1000
                                  ? (missionDistM / 1000).toFixed(2) + " km"
                                  : Math.round(missionDistM) + " m"
                            color: Theme.accentBlue; font.pixelSize: Theme.fontValueSmall; font.weight: Font.Bold; font.family: "monospace"
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Map control buttons
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    spacing: 5

                    Repeater {
                        model: [
                            { label: "CENTER ON DRONE", action: "center" },
                            { label: "ZOOM  +",         action: "zoomin" },
                            { label: "ZOOM  –",         action: "zoomout" }
                        ]
                        Rectangle {
                            Layout.fillWidth: true; height: Theme.minTouchSize - 4; radius: 4
                            color: mapBtnMa.pressed ? Qt.rgba(0.22, 0.55, 1.0, 0.10) : Theme.bgElevated
                            border.color: mapBtnMa.pressed ? Theme.accentBlue : Theme.border; border.width: 1
                            Behavior on color { ColorAnimation { duration: 80 } }
                            Behavior on border.color { ColorAnimation { duration: 80 } }
                            scale: mapBtnMa.pressed ? 0.97 : 1.0
                            Behavior on scale { NumberAnimation { duration: 70 } }
                            Text { anchors.centerIn: parent; text: modelData.label; color: Theme.textSecondary; font.pixelSize: Theme.fontButton; font.weight: Font.Medium }
                            MouseArea {
                                id: mapBtnMa; anchors.fill: parent
                                onClicked: {
                                    if (modelData.action === "center" && hasDroneFix)
                                        theMap.center = QtPositioning.coordinate(GCSState.droneLat, GCSState.droneLon)
                                    else if (modelData.action === "zoomin")
                                        theMap.zoomLevel = Math.min(20, theMap.zoomLevel + 1)
                                    else if (modelData.action === "zoomout")
                                        theMap.zoomLevel = Math.max(2, theMap.zoomLevel - 1)
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

        // ── MAP ───────────────────────────────────────────────────────────────
        Map {
            id: theMap
            Layout.fillWidth: true
            Layout.fillHeight: true
            plugin: Plugin {
                name: "osm"
                PluginParameter { name: "osm.useragent"; value: "KL-GCS/1.0" }
            }
            center: defaultCenter
            zoomLevel: 14
            copyrightsVisible: false

            // Follow drone on first fix
            Connections {
                target: GCSState
                function onDronePositionChanged() {
                    if (hasDroneFix && theMap.center.latitude === defaultCenter.latitude
                            && theMap.center.longitude === defaultCenter.longitude) {
                        theMap.center = QtPositioning.coordinate(GCSState.droneLat, GCSState.droneLon)
                    }
                }
            }

            // Drone marker
            MapQuickItem {
                id: droneMarker
                visible: hasDroneFix
                coordinate: QtPositioning.coordinate(GCSState.droneLat, GCSState.droneLon)
                anchorPoint.x: 10; anchorPoint.y: 10
                sourceItem: Canvas {
                    width: 20; height: 20
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, 20, 20)
                        var cx = 10, cy = 10
                        ctx.save()
                        ctx.translate(cx, cy)
                        ctx.rotate(GCSState.heading * Math.PI / 180)
                        ctx.fillStyle = "#e3d049"
                        ctx.beginPath()
                        ctx.moveTo(0, -9); ctx.lineTo(6, 7); ctx.lineTo(0, 4); ctx.lineTo(-6, 7)
                        ctx.closePath(); ctx.fill()
                        ctx.restore()
                    }
                    Connections {
                        target: GCSState
                        function onDroneStateChanged() { parent.requestPaint() }
                        function onDronePositionChanged() { parent.requestPaint() }
                    }
                }
            }

            // Waypoint markers
            MapItemView {
                model: GCSState.waypoints
                delegate: MapQuickItem {
                    coordinate: QtPositioning.coordinate(modelData.lat || 0, modelData.lon || 0)
                    anchorPoint.x: 12; anchorPoint.y: 12
                    sourceItem: Rectangle {
                        width: 24; height: 24; radius: 12
                        color: Theme.bgPrimary; border.color: Theme.accentBlue; border.width: 2
                        Text {
                            anchors.centerIn: parent
                            text: index + 1
                            color: Theme.accentBlue; font.pixelSize: 10; font.weight: Font.Bold
                        }
                    }
                }
            }

            // Waypoint path line
            MapPolyline {
                visible: GCSState.waypoints.length >= 2
                line.color: Theme.accentBlue
                line.width: 2
                path: {
                    var pts = []
                    for (var i = 0; i < GCSState.waypoints.length; i++)
                        pts.push(QtPositioning.coordinate(GCSState.waypoints[i].lat || 0, GCSState.waypoints[i].lon || 0))
                    return pts
                }
            }

            // Map gesture handler
            PinchHandler { }
            WheelHandler { acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad }
        }
    }
}
