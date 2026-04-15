import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import PICODE

Rectangle {
    color: Theme.bgPrimary

    readonly property int touchTarget: 44
    readonly property int touchTargetLarge: 52

    property int selectedWpIndex: -1
    property bool poiMode: false
    property int selectedPoiIndex: -1
    property bool draggingMarker: false
    property string missionProfile: "WAYPOINT"

    property var profileSettings: ({
        "WAYPOINT": { cruise: 14, alt: 35, loiter: 8, wpType: "WAYPOINT" },
        "LOITER":   { cruise: 10, alt: 40, loiter: 20, wpType: "LOITER" },
        "CIRCLE":   { cruise: 9,  alt: 42, loiter: 18, wpType: "LOITER" },
        "SPLINE":   { cruise: 12, alt: 38, loiter: 8, wpType: "SPLINE_WP" },
        "LAND":     { cruise: 8,  alt: 18, loiter: 5, wpType: "LAND" }
    })

    readonly property var defaultCenter: QtPositioning.coordinate(52.3731, 4.8965)

    readonly property var wpTypes: ["TAKEOFF", "WAYPOINT", "LOITER", "SPLINE_WP", "LAND", "ROI"]

    function profileDefaultType() {
        return profileSettings[missionProfile].wpType
    }

    function profileDefaultAlt() {
        return profileSettings[missionProfile].alt
    }

    function updateProfileSetting(key, value) {
        var next = {}
        for (var k in profileSettings) {
            next[k] = {
                cruise: profileSettings[k].cruise,
                alt: profileSettings[k].alt,
                loiter: profileSettings[k].loiter,
                wpType: profileSettings[k].wpType
            }
        }
        next[missionProfile][key] = value
        profileSettings = next
    }

    function updateWaypoint(index, key, value) {
        var wps = []
        for (var i = 0; i < GCSState.waypoints.length; i++) {
            var wp = GCSState.waypoints[i]
            if (i === index) {
                var updated = {}
                for (var k in wp) updated[k] = wp[k]
                updated[key] = value
                wps.push(updated)
            } else {
                wps.push(wp)
            }
        }
        GCSState.waypoints = wps
    }

    function updatePoi(index, key, value) {
        var list = []
        for (var i = 0; i < GCSState.pois.length; i++) {
            var p = GCSState.pois[i]
            if (i === index) {
                var updated = {}
                for (var k in p) updated[k] = p[k]
                updated[key] = value
                list.push(updated)
            } else {
                list.push(p)
            }
        }
        GCSState.pois = list
    }

    // Catmull-Rom spline interpolation
    function catmullRom(p0, p1, p2, p3, t) {
        var t2 = t * t
        var t3 = t2 * t
        return 0.5 * ((2 * p1) +
            (-p0 + p2) * t +
            (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 +
            (-p0 + 3 * p1 - 3 * p2 + p3) * t3)
    }

    function computePath() {
        var wps = GCSState.waypoints
        if (wps.length < 2) return []

        var pts = []
        for (var i = 0; i < wps.length; i++) {
            var wp = wps[i]
            if (wp.type === "SPLINE_WP" && i < wps.length - 1) {
                var p0 = i > 0 ? wps[i - 1] : wp
                var p1 = wp
                var p2 = wps[i + 1]
                var p3 = i < wps.length - 2 ? wps[i + 2] : p2

                var steps = 12
                for (var s = 0; s <= steps; s++) {
                    var t = s / steps
                    var lat = catmullRom(p0.lat || 0, p1.lat || 0, p2.lat || 0, p3.lat || 0, t)
                    var lon = catmullRom(p0.lon || 0, p1.lon || 0, p2.lon || 0, p3.lon || 0, t)
                    pts.push(QtPositioning.coordinate(lat, lon))
                }
            } else {
                pts.push(QtPositioning.coordinate(wp.lat || 0, wp.lon || 0))
            }
        }
        return pts
    }

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

                // Tap-to-add waypoint or POI
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    enabled: !draggingMarker
                    onClicked: function(mouse) {
                        var coord = missionMap.toCoordinate(Qt.point(mouse.x, mouse.y))

                        if (poiMode) {
                            var pois = []
                            for (var j = 0; j < GCSState.pois.length; j++)
                                pois.push(GCSState.pois[j])
                            pois.push({ lat: coord.latitude, lon: coord.longitude, alt: 20, label: "POI " + (pois.length + 1) })
                            GCSState.pois = pois
                            selectedPoiIndex = pois.length - 1
                        } else {
                            var wps = []
                            for (var i = 0; i < GCSState.waypoints.length; i++)
                                wps.push(GCSState.waypoints[i])
                            var wpType = wps.length === 0 ? "TAKEOFF" : profileDefaultType()
                            wps.push({ lat: coord.latitude, lon: coord.longitude, alt: profileDefaultAlt(), type: wpType })
                            GCSState.waypoints = wps
                            selectedWpIndex = wps.length - 1
                        }
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

                // WP path line (with spline interpolation)
                MapPolyline {
                    visible: GCSState.waypoints.length >= 2
                    line.color: Theme.accentBlue; line.width: 2
                    path: computePath()
                }

                // WP markers (draggable)
                MapItemView {
                    model: GCSState.waypoints
                    delegate: MapQuickItem {
                        id: wpMarker
                        coordinate: QtPositioning.coordinate(modelData.lat || 0, modelData.lon || 0)
                        anchorPoint.x: 20; anchorPoint.y: 20
                        sourceItem: Rectangle {
                            width: 40; height: 40; radius: 20
                            color: selectedWpIndex === index ? Theme.accentYellow
                                 : modelData.type === "SPLINE_WP" ? Qt.rgba(0.5, 0.84, 1.0, 0.15)
                                 : Theme.bgSecondary
                            border.color: selectedWpIndex === index ? Theme.accentYellow
                                        : modelData.type === "SPLINE_WP" ? Theme.accentBlue
                                        : Theme.accentBlue
                            border.width: 2
                            Text {
                                anchors.centerIn: parent
                                text: index + 1
                                color: selectedWpIndex === index ? Theme.bgPrimary : Theme.accentBlue
                                font.pixelSize: 13; font.weight: Font.Bold
                            }
                            MouseArea {
                                anchors.fill: parent
                                drag.target: parent
                                preventStealing: true
                                onClicked: { selectedWpIndex = index; selectedPoiIndex = -1 }
                                onPressed: draggingMarker = true
                                onReleased: {
                                    draggingMarker = false
                                    // Convert final position back to coordinate
                                    var itemPos = mapToItem(missionMap, parent.x + 20, parent.y + 20)
                                    var newCoord = missionMap.toCoordinate(Qt.point(itemPos.x, itemPos.y))
                                    if (newCoord.isValid) {
                                        updateWaypoint(index, "lat", newCoord.latitude)
                                        updateWaypoint(index, "lon", newCoord.longitude)
                                    }
                                }
                            }
                        }
                    }
                }

                // POI markers (diamond shape)
                MapItemView {
                    model: GCSState.pois
                    delegate: MapQuickItem {
                        coordinate: QtPositioning.coordinate(modelData.lat || 0, modelData.lon || 0)
                        anchorPoint.x: 17; anchorPoint.y: 17
                        sourceItem: Item {
                            width: 34; height: 34
                            Rectangle {
                                anchors.centerIn: parent
                                width: 24; height: 24
                                rotation: 45
                                color: selectedPoiIndex === index ? Theme.statusWarn : Qt.rgba(1, 0.7, 0.28, 0.15)
                                border.color: Theme.statusWarn
                                border.width: 2
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "P"
                                color: selectedPoiIndex === index ? Theme.bgPrimary : Theme.statusWarn
                                font.pixelSize: 11; font.weight: Font.Bold
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: { selectedPoiIndex = index; selectedWpIndex = -1 }
                            }
                        }
                    }
                }

                // ROI lines (from POI to linked waypoints)
                Repeater {
                    model: GCSState.pois
                    MapPolyline {
                        visible: true
                        line.color: Qt.rgba(1, 0.7, 0.28, 0.4); line.width: 1
                        path: [
                            QtPositioning.coordinate(modelData.lat || 0, modelData.lon || 0),
                            // Draw to center of mission if POI exists
                            GCSState.waypoints.length > 0
                                ? QtPositioning.coordinate(GCSState.waypoints[0].lat || 0, GCSState.waypoints[0].lon || 0)
                                : QtPositioning.coordinate(modelData.lat || 0, modelData.lon || 0)
                        ]
                    }
                }

                PinchHandler { }
                WheelHandler { acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad }
            }

            // Mode indicator
            Rectangle {
                anchors { top: parent.top; left: parent.left; margins: 8 }
                visible: poiMode
                color: Qt.rgba(1, 0.7, 0.28, 0.15); radius: 4
                width: poiModeText.width + 16; height: poiModeText.height + 8
                border.color: Theme.statusWarn; border.width: 1
                Text {
                    id: poiModeText
                    anchors.centerIn: parent
                    text: "POI MODE — TAP TO PLACE"
                    color: Theme.statusWarn; font.pixelSize: 10; font.weight: Font.SemiBold
                }
            }

            // Hint overlay
            Rectangle {
                anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 8 }
                color: Qt.rgba(0, 0, 0, 0.55); radius: 4
                width: hintText.width + 16; height: hintText.height + 8
                Text {
                    id: hintText
                    anchors.centerIn: parent
                    text: poiMode
                        ? "TAP MAP TO ADD POI  ·  TAP MARKER TO SELECT"
                        : "TAP MAP TO ADD WP  ·  DRAG MARKER TO MOVE"
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
                            + (GCSState.pois.length > 0 ? "  " + GCSState.pois.length + " POI" : "")
                        color: GCSState.waypoints.length > 0 ? Theme.accentYellow : Theme.textDisabled
                        font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold; font.family: "monospace"
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 124
                color: Theme.bgSecondary
                border.color: Theme.border
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Text {
                        text: "MODE PROFILES"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSectionLabel
                        font.weight: Font.SemiBold
                        font.letterSpacing: 0.6
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Repeater {
                            model: ["WAYPOINT", "LOITER", "CIRCLE", "SPLINE", "LAND"]
                            Rectangle {
                                Layout.fillWidth: true
                                height: touchTarget
                                radius: 3
                                property bool selected: missionProfile === modelData
                                color: selected ? Qt.rgba(Theme.accentYellow.r, Theme.accentYellow.g, Theme.accentYellow.b, 0.12) : Theme.bgElevated
                                border.color: selected ? Theme.accentYellow : Theme.border
                                border.width: selected ? 2 : 1
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: parent.selected ? Theme.accentYellow : Theme.textSecondary
                                    font.pixelSize: 11
                                    font.weight: Font.SemiBold
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: missionProfile = modelData
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text { text: "CRUISE SPEED"; color: Theme.textDisabled; font.pixelSize: 10; Layout.preferredWidth: 78 }
                        Slider {
                            id: speedProfileSlider
                            from: 4; to: 30; stepSize: 1
                            value: profileSettings[missionProfile].cruise
                            height: touchTarget
                            Layout.fillWidth: true
                            onMoved: updateProfileSetting("cruise", Math.round(value))
                        }
                        Text {
                            text: profileSettings[missionProfile].cruise + "m/s"
                            color: Theme.accentBlue
                            font.pixelSize: 10
                            font.family: "monospace"
                            Layout.preferredWidth: 50
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            text: "WAYPOINT TYPE " + profileDefaultType() + "   ALTITUDE " + profileSettings[missionProfile].alt + "m   LOITER TIME " + profileSettings[missionProfile].loiter + "s"
                            color: Theme.textSecondary
                            font.pixelSize: 10
                            font.family: "monospace"
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // WP + POI list
            ListView {
                id: wpEditor
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: GCSState.waypoints
                spacing: 0

                delegate: Rectangle {
                    width: wpEditor.width
                    height: selectedWpIndex === index ? 230 : 84
                    Behavior on height { NumberAnimation { duration: 120 } }
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
                        anchors { fill: parent; leftMargin: selectedWpIndex === index ? 11 : 8; rightMargin: 8; topMargin: 6; bottomMargin: 6 }
                        spacing: 3

                        // Summary row (always visible)
                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Text {
                                text: "WP" + (index + 1)
                                color: Theme.accentYellow; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold; font.family: "monospace"
                                Layout.preferredWidth: 36
                            }
                            Text {
                                text: modelData.type ? modelData.type : "WAYPOINT"
                                color: modelData.type === "SPLINE_WP" ? Theme.accentBlue : Theme.textPrimary
                                font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium
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

                        // ── EXPANDED EDIT CONTROLS ──────────────────────────
                        ColumnLayout {
                            visible: selectedWpIndex === index
                            Layout.fillWidth: true
                            spacing: 4

                            // Type selector
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Repeater {
                                    model: wpTypes
                                    Rectangle {
                                        Layout.fillWidth: true; height: 36; radius: 3
                                        color: modelData === (GCSState.waypoints[index] ? GCSState.waypoints[index].type : "") ? Theme.accentYellow : Theme.bgElevated
                                        border.color: modelData === (GCSState.waypoints[index] ? GCSState.waypoints[index].type : "") ? Theme.accentYellow : Theme.border
                                        border.width: 1
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.length > 5 ? modelData.substring(0, 5) : modelData
                                            color: modelData === (GCSState.waypoints[index] ? GCSState.waypoints[index].type : "") ? Theme.bgPrimary : Theme.textSecondary
                                            font.pixelSize: 10; font.weight: Font.SemiBold
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: updateWaypoint(selectedWpIndex, "type", modelData)
                                        }
                                    }
                                }
                            }

                            // Altitude slider + input
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                Layout.preferredHeight: touchTarget

                                Text {
                                    text: "ALT"
                                    color: Theme.textSecondary; font.pixelSize: 10; font.weight: Font.Medium
                                    Layout.preferredWidth: 28
                                }

                                Slider {
                                    id: altSlider
                                    from: 0; to: 200; stepSize: 1
                                    value: modelData.alt !== undefined ? modelData.alt : 20
                                    Layout.fillWidth: true
                                    height: touchTarget

                                    background: Item {
                                        x: altSlider.leftPadding
                                        y: altSlider.topPadding + altSlider.availableHeight / 2 - 3
                                        width: altSlider.availableWidth; height: 6
                                        Rectangle { width: parent.width; height: parent.height; radius: 3; color: Theme.bgElevated; border.color: Theme.border; border.width: 1 }
                                        Rectangle { width: altSlider.visualPosition * parent.width; height: parent.height; radius: 3; color: Theme.accentBlue; opacity: 0.85 }
                                    }
                                    handle: Rectangle {
                                        x: altSlider.leftPadding + altSlider.visualPosition * (altSlider.availableWidth - width)
                                        y: altSlider.topPadding + altSlider.availableHeight / 2 - height / 2
                                        width: 18; height: 18; radius: 9
                                        color: Theme.accentYellow; border.color: Theme.bgPrimary; border.width: 2
                                    }

                                    onMoved: updateWaypoint(selectedWpIndex, "alt", Math.round(value))
                                }

                                Text {
                                    text: (modelData.alt !== undefined ? modelData.alt : 20) + "m"
                                    color: Theme.accentBlue; font.pixelSize: 10; font.family: "monospace"; font.weight: Font.Bold
                                    Layout.preferredWidth: 36
                                    horizontalAlignment: Text.AlignRight
                                }
                            }

                            // Lat/Lon edit row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                Layout.preferredHeight: touchTarget

                                Text { text: "LAT"; color: Theme.textSecondary; font.pixelSize: 9; Layout.preferredWidth: 24 }
                                TextField {
                                    Layout.fillWidth: true; height: touchTarget
                                    text: modelData.lat !== undefined ? modelData.lat.toFixed(6) : ""
                                    color: Theme.textPrimary; font.pixelSize: 10; font.family: "monospace"
                                    background: Rectangle { color: Theme.bgElevated; border.color: Theme.border; radius: 2 }
                                    onEditingFinished: {
                                        var v = parseFloat(text)
                                        if (!isNaN(v)) updateWaypoint(selectedWpIndex, "lat", v)
                                    }
                                }

                                Text { text: "LON"; color: Theme.textSecondary; font.pixelSize: 9; Layout.preferredWidth: 24 }
                                TextField {
                                    Layout.fillWidth: true; height: touchTarget
                                    text: modelData.lon !== undefined ? modelData.lon.toFixed(6) : ""
                                    color: Theme.textPrimary; font.pixelSize: 10; font.family: "monospace"
                                    background: Rectangle { color: Theme.bgElevated; border.color: Theme.border; radius: 2 }
                                    onEditingFinished: {
                                        var v = parseFloat(text)
                                        if (!isNaN(v)) updateWaypoint(selectedWpIndex, "lon", v)
                                    }
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        z: -1
                        onClicked: { selectedWpIndex = index; selectedPoiIndex = -1 }
                    }
                }
            }

            // POI list (if any)
            ListView {
                id: poiEditor
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(GCSState.pois.length * 64, 128)
                visible: GCSState.pois.length > 0
                clip: true
                model: GCSState.pois
                spacing: 0

                delegate: Rectangle {
                    width: poiEditor.width; height: 84
                    color: selectedPoiIndex === index ? Qt.rgba(1, 0.7, 0.28, 0.06) : Theme.bgSecondary
                    border.color: selectedPoiIndex === index ? Theme.statusWarn : Theme.border
                    border.width: 1

                    Rectangle {
                        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                        width: 3; color: Theme.statusWarn
                        visible: selectedPoiIndex === index
                    }

                    ColumnLayout {
                        anchors { fill: parent; leftMargin: 11; rightMargin: 8; topMargin: 6; bottomMargin: 6 }
                        spacing: 3

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            // Diamond icon
                            Rectangle {
                                width: 12; height: 12; rotation: 45
                                color: Theme.statusWarn
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Text {
                                text: modelData.label || ("POI " + (index + 1))
                                color: Theme.statusWarn; font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Bold
                                Layout.fillWidth: true
                            }
                            Text {
                                text: modelData.alt + " m"
                                color: Theme.accentBlue; font.pixelSize: Theme.fontSectionLabel; font.family: "monospace"
                            }
                        }
                        RowLayout {
                            spacing: 8
                            Text {
                                text: (modelData.lat !== undefined ? modelData.lat.toFixed(5) : "--")
                                      + ", " + (modelData.lon !== undefined ? modelData.lon.toFixed(5) : "--")
                                color: Theme.textDisabled; font.pixelSize: 10; font.family: "monospace"
                            }
                            // Height edit (compact)
                            Text { text: "H:"; color: Theme.textSecondary; font.pixelSize: 9 }
                            TextField {
                                width: 64; height: 32
                                text: modelData.alt !== undefined ? modelData.alt.toString() : "20"
                                color: Theme.textPrimary; font.pixelSize: 10; font.family: "monospace"
                                background: Rectangle { color: Theme.bgElevated; border.color: Theme.border; radius: 2 }
                                onEditingFinished: {
                                    var v = parseFloat(text)
                                    if (!isNaN(v)) updatePoi(index, "alt", v)
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent; z: -1
                        onClicked: { selectedPoiIndex = index; selectedWpIndex = -1 }
                    }
                }
            }

            // ADD / DEL / POI buttons
            Rectangle {
                Layout.fillWidth: true; height: 72
                color: Theme.bgSecondary

                RowLayout {
                    anchors { fill: parent; margins: 8 }
                    spacing: 6

                    // ADD WP
                    Rectangle {
                        Layout.fillWidth: true; height: touchTargetLarge; radius: 4
                        color: addMa.pressed ? Qt.rgba(0.89, 0.82, 0.29, 0.10) : Theme.bgElevated
                        border.color: addMa.pressed ? Theme.accentYellow : Theme.border; border.width: 1
                        scale: addMa.pressed ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 70 } }
                        Text { anchors.centerIn: parent; text: "+ WP"; color: Theme.accentYellow; font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold }
                        MouseArea {
                            id: addMa; anchors.fill: parent
                            onClicked: {
                                var wps = []
                                for (var i = 0; i < GCSState.waypoints.length; i++)
                                    wps.push(GCSState.waypoints[i])
                                wps.push({ lat: missionMap.center.latitude, lon: missionMap.center.longitude, alt: profileDefaultAlt(), type: profileDefaultType() })
                                GCSState.waypoints = wps
                                selectedWpIndex = wps.length - 1
                                poiMode = false
                            }
                        }
                    }

                    // DEL
                    Rectangle {
                        Layout.fillWidth: true; height: touchTargetLarge; radius: 4
                        property bool canDel: selectedWpIndex >= 0 || selectedPoiIndex >= 0
                        color: canDel && delMa.pressed ? Qt.rgba(1, 0.36, 0.36, 0.08) : Theme.bgElevated
                        border.color: canDel ? (delMa.pressed ? Theme.statusCrit : Theme.border) : Theme.bgElevated; border.width: 1
                        scale: delMa.pressed && canDel ? 0.97 : 1.0
                        Behavior on scale { NumberAnimation { duration: 70 } }
                        Text {
                            anchors.centerIn: parent; text: "- DEL"
                            color: parent.canDel ? Theme.statusCrit : Theme.textDisabled
                            font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold
                        }
                        MouseArea {
                            id: delMa; anchors.fill: parent
                            enabled: parent.canDel
                            onClicked: {
                                if (selectedPoiIndex >= 0) {
                                    var pois = []
                                    for (var j = 0; j < GCSState.pois.length; j++)
                                        if (j !== selectedPoiIndex) pois.push(GCSState.pois[j])
                                    GCSState.pois = pois
                                    selectedPoiIndex = Math.min(selectedPoiIndex, pois.length - 1)
                                } else if (selectedWpIndex >= 0) {
                                    var wps = []
                                    for (var i = 0; i < GCSState.waypoints.length; i++)
                                        if (i !== selectedWpIndex) wps.push(GCSState.waypoints[i])
                                    GCSState.waypoints = wps
                                    selectedWpIndex = Math.min(selectedWpIndex, wps.length - 1)
                                }
                            }
                        }
                    }

                    // POI mode toggle
                    Rectangle {
                        Layout.fillWidth: true; height: touchTargetLarge; radius: 4
                        color: poiMode ? Qt.rgba(1, 0.7, 0.28, 0.10) : Theme.bgElevated
                        border.color: poiMode ? Theme.statusWarn : Theme.border
                        border.width: poiMode ? 2 : 1
                        Text {
                            anchors.centerIn: parent; text: "POI"
                            color: poiMode ? Theme.statusWarn : Theme.textSecondary
                            font.pixelSize: Theme.fontButton; font.weight: Font.SemiBold
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: poiMode = !poiMode
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
                    Layout.fillWidth: true; height: 72
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
            + (GCSState.pois.length > 0 ? "\n\nIncludes " + GCSState.pois.length + " POI(s) as ROI commands." : "")
        confirmLabel: "UPLOAD"
        destructive: false
        onConfirmed: GCSState.sendMissionUpload()
    }

    ConfirmOverlay {
        id: clearConfirm
        title: "CLEAR MISSION"
        body: "All waypoints and POIs will be deleted from the editor. The drone's stored mission is not affected until you upload."
        confirmLabel: "CLEAR"
        destructive: true
        onConfirmed: { GCSState.waypoints = []; GCSState.pois = []; selectedWpIndex = -1; selectedPoiIndex = -1 }
    }
}
