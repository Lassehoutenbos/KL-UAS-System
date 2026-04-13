import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PICODE

Rectangle {
    color: Theme.bgPrimary

    property string searchText:    ""
    property string activeGroup:   "ALL"
    property string editParamName: ""
    property double editParamValue: 0.0
    property string editInputText: ""

    // Param group prefix filters
    readonly property var groups: [
        { label: "ALL",      prefix: "" },
        { label: "ATTITUDE", prefix: "ATC_" },
        { label: "TUNING",   prefix: "TUNE" },
        { label: "BATTERY",  prefix: "BATT" },
        { label: "GPS",      prefix: "GPS_" },
        { label: "COMPASS",  prefix: "COMPASS" },
        { label: "FAILSAFE", prefix: "FS_" },
        { label: "RADIO",    prefix: "RC_" },
        { label: "MOTORS",   prefix: "MOT_" }
    ]

    // Filtered params list
    readonly property var filteredParams: {
        var list = []
        var groupPrefix = ""
        for (var g = 0; g < groups.length; g++) {
            if (groups[g].label === activeGroup) { groupPrefix = groups[g].prefix; break }
        }
        for (var i = 0; i < GCSState.params.length; i++) {
            var p = GCSState.params[i]
            var name = p.name ? p.name.toString() : ""
            if (groupPrefix && !name.startsWith(groupPrefix)) continue
            if (searchText && !name.toUpperCase().includes(searchText.toUpperCase())) continue
            list.push(p)
        }
        return list
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

                // Search header
                Item {
                    Layout.fillWidth: true; height: 32
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentBlue }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "SEARCH"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                // Search field
                Rectangle {
                    Layout.fillWidth: true; height: 44
                    Layout.leftMargin: 8; Layout.rightMargin: 8; Layout.topMargin: 6; Layout.bottomMargin: 4
                    radius: 4
                    color: Theme.bgElevated; border.color: Theme.accentBlue; border.width: 1
                    RowLayout {
                        anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
                        spacing: 6
                        Text { text: "⌕"; color: Theme.textDisabled; font.pixelSize: 14 }
                        TextInput {
                            Layout.fillWidth: true
                            text: searchText
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSectionLabel
                            onTextChanged: searchText = text
                            clip: true
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: 2 }

                // Groups header
                Item {
                    Layout.fillWidth: true; height: 32
                    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentYellow }
                    Text {
                        anchors { left: parent.left; leftMargin: 11; verticalCenter: parent.verticalCenter }
                        text: "GROUPS"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    topMargin: 4; bottomMargin: 4; leftMargin: 6; rightMargin: 6
                    clip: true
                    model: groups
                    spacing: 2
                    delegate: Rectangle {
                        width: parent ? parent.width - 12 : 0
                        height: Theme.minTouchSize - 8
                        radius: 4
                        color: activeGroup === modelData.label ? Qt.rgba(0.89, 0.82, 0.29, 0.10) : "transparent"
                        border.color: activeGroup === modelData.label ? Theme.accentYellow : "transparent"
                        border.width: 1
                        Rectangle {
                            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                            width: 3; radius: 1.5; color: Theme.accentYellow
                            visible: activeGroup === modelData.label
                        }
                        RowLayout {
                            anchors { fill: parent; leftMargin: activeGroup === modelData.label ? 10 : 8 }
                            spacing: 6
                            Text {
                                text: modelData.label
                                color: activeGroup === modelData.label ? Theme.accentYellow : Theme.textSecondary
                                font.pixelSize: Theme.fontSectionLabel; font.weight: activeGroup === modelData.label ? Font.SemiBold : Font.Medium
                                Behavior on color { ColorAnimation { duration: 100 } }
                            }
                        }
                        MouseArea { anchors.fill: parent; onClicked: { activeGroup = modelData.label; searchText = "" } }
                    }
                }
            }
        }

        Rectangle { width: 1; Layout.fillHeight: true; color: Theme.border }

        // ── RIGHT: PARAM LIST + ACTIONS ───────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Param count header
            Item {
                Layout.fillWidth: true; height: 32
                Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 3; color: Theme.accentYellow }
                RowLayout {
                    anchors { fill: parent; leftMargin: 11; rightMargin: 10 }
                    Text {
                        text: "PARAMETERS"
                        color: Theme.textSecondary; font.pixelSize: Theme.fontPageTitle; font.weight: Font.SemiBold; font.letterSpacing: 0.8
                        Layout.fillWidth: true
                    }
                    Rectangle {
                        width: paramCountLabel.width + 12; height: 20; radius: 3
                        color: Theme.bgElevated; border.color: Theme.border; border.width: 1
                        Text {
                            id: paramCountLabel
                            anchors.centerIn: parent
                            text: filteredParams.length + " / " + GCSState.params.length
                            color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel; font.family: "monospace"
                        }
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            // Empty state
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true
                visible: GCSState.params.length === 0
                Text {
                    anchors.centerIn: parent
                    text: "No parameters loaded.\nPress REFRESH to download from drone."
                    color: Theme.textDisabled; font.pixelSize: Theme.fontSectionLabel
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Parameter list
            ListView {
                id: paramList
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: GCSState.params.length > 0
                clip: true
                model: filteredParams
                spacing: 0

                delegate: Rectangle {
                    width: paramList.width
                    height: 56
                    color: index % 2 === 0 ? Theme.bgPrimary : Theme.bgSecondary
                    border.color: modelData.dirty ? Theme.statusWarn : "transparent"
                    border.width: modelData.dirty ? 1 : 0

                    RowLayout {
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                        spacing: 8

                        // Dirty indicator
                        Rectangle {
                            width: 4; height: 36; radius: 2
                            color: modelData.dirty ? Theme.statusWarn : "transparent"
                        }

                        Text {
                            text: modelData.name
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSectionLabel; font.weight: Font.Medium
                            font.family: "monospace"
                            Layout.preferredWidth: 220
                            elide: Text.ElideRight
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: {
                                var v = modelData.value
                                if (v === undefined || v === null) return "--"
                                return parseFloat(v).toFixed(3)
                            }
                            color: modelData.dirty ? Theme.statusWarn : Theme.textPrimary
                            font.pixelSize: Theme.fontValueSmall; font.weight: Font.SemiBold
                            font.family: "monospace"
                            Layout.preferredWidth: 90
                            horizontalAlignment: Text.AlignRight

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    editParamName  = modelData.name
                                    editParamValue = modelData.value !== undefined ? parseFloat(modelData.value) : 0.0
                                    editInputText  = editParamValue.toFixed(4)
                                    paramEditOverlay.visible = true
                                }
                            }
                        }

                        // Decrement button
                        Rectangle {
                            id: decBtn
                            width: 52; height: 44; radius: 4
                            color: decMa.pressed ? Theme.bgElevated : "transparent"
                            border.color: decMa.containsMouse ? Theme.border : "transparent"; border.width: 1
                            Behavior on color { ColorAnimation { duration: 80 } }
                            scale: decMa.pressed ? 0.92 : 1.0
                            Behavior on scale { NumberAnimation { duration: 70 } }
                            Text { anchors.centerIn: parent; text: "–"; color: Theme.textSecondary; font.pixelSize: 18; font.weight: Font.Bold }
                            MouseArea {
                                id: decMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: {
                                    var newV = parseFloat(modelData.value) - 0.01
                                    GCSState.setParamDirty(modelData.name, Math.round(newV * 10000) / 10000)
                                }
                            }
                        }

                        // Increment button
                        Rectangle {
                            id: incBtn
                            width: 52; height: 44; radius: 4
                            color: incMa.pressed ? Theme.bgElevated : "transparent"
                            border.color: incMa.containsMouse ? Theme.border : "transparent"; border.width: 1
                            Behavior on color { ColorAnimation { duration: 80 } }
                            scale: incMa.pressed ? 0.92 : 1.0
                            Behavior on scale { NumberAnimation { duration: 70 } }
                            Text { anchors.centerIn: parent; text: "+"; color: Theme.accentYellow; font.pixelSize: 18; font.weight: Font.Bold }
                            MouseArea {
                                id: incMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: {
                                    var newV = parseFloat(modelData.value) + 0.01
                                    GCSState.setParamDirty(modelData.name, Math.round(newV * 10000) / 10000)
                                }
                            }
                        }
                    }
                }
            }

            // WRITE / REFRESH buttons
            Rectangle {
                Layout.fillWidth: true; height: 64
                color: Theme.bgSecondary; border.color: Theme.border; border.width: 1

                RowLayout {
                    anchors { fill: parent; margins: 8 }
                    spacing: 8

                    BigButton {
                        label: "WRITE TO DRONE"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        active: true
                        onClicked: writeConfirm.open()
                    }

                    BigButton {
                        label: "REFRESH FROM DRONE"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        onClicked: GCSState.sendParamRefresh()
                    }
                }
            }
        }
    }

    // ── Parameter edit overlay ────────────────────────────────────────────────
    Rectangle {
        id: paramEditOverlay
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6)
        visible: false
        z: 50

        MouseArea { anchors.fill: parent; onClicked: paramEditOverlay.visible = false }

        Rectangle {
            anchors.centerIn: parent
            width: 440; height: 280
            color: Theme.bgSecondary; border.color: Theme.border; border.width: 1; radius: 8

            ColumnLayout {
                anchors { fill: parent; margins: 16 }
                spacing: 10

                Text { text: editParamName; color: Theme.textPrimary; font.pixelSize: 16; font.weight: Font.SemiBold }
                Text { text: "Current:  " + editParamValue.toFixed(4); color: Theme.textSecondary; font.pixelSize: Theme.fontSectionLabel }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                Rectangle {
                    Layout.fillWidth: true; height: 52; radius: 4
                    color: Theme.bgElevated; border.color: Theme.accentBlue; border.width: 1
                    TextInput {
                        anchors { fill: parent; leftMargin: 12; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        text: editInputText
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontValueSmall; font.weight: Font.SemiBold
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: DoubleValidator { }
                        onTextChanged: editInputText = text
                        clip: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 8

                    BigButton {
                        label: "CANCEL"
                        Layout.preferredWidth: 180; Layout.preferredHeight: 56
                        onClicked: paramEditOverlay.visible = false
                    }

                    BigButton {
                        label: "SET"
                        active: true
                        Layout.preferredWidth: 180; Layout.preferredHeight: 56
                        onClicked: {
                            var v = parseFloat(editInputText)
                            if (!isNaN(v)) GCSState.setParamDirty(editParamName, v)
                            paramEditOverlay.visible = false
                        }
                    }
                }
            }
        }
    }

    // ── Write confirm overlay ─────────────────────────────────────────────────
    ConfirmOverlay {
        id: writeConfirm
        title: "WRITE PARAMS TO DRONE"
        body: "All changed parameters will be sent to the drone. Ensure the drone is disarmed before writing critical params."
        confirmLabel: "WRITE"
        destructive: false
        onConfirmed: {
            for (var i = 0; i < GCSState.params.length; i++) {
                var p = GCSState.params[i]
                if (p.dirty) GCSState.sendParamWrite(p.name, p.value)
            }
        }
    }
}
