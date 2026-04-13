pragma Singleton
import QtQuick

QtObject {
    // Backgrounds
    readonly property color bgPrimary:   "#0b0d10"
    readonly property color bgSecondary: "#12161b"
    readonly property color bgElevated:  "#1a2027"

    // Text
    readonly property color textPrimary:   "#e8edf2"
    readonly property color textSecondary: "#8f9baa"
    readonly property color textDisabled:  "#5c6673"

    // Accents
    readonly property color accentYellow: "#e3d049"
    readonly property color accentBlue:   "#7fd6ff"

    // Status
    readonly property color statusOk:   "#62d48f"
    readonly property color statusWarn: "#ffb347"
    readonly property color statusCrit: "#ff5b5b"

    // Borders
    readonly property color border: "#2a313a"

    // Typography sizes
    readonly property int fontPageTitle:    13
    readonly property int fontSectionLabel: 11
    readonly property int fontValueLarge:   32
    readonly property int fontValueMedium:  20
    readonly property int fontValueSmall:   14
    readonly property int fontButton:       13
    readonly property int fontUnit:         11

    // Touch target minimums
    readonly property int minTouchSize: 56

    // Layout
    readonly property int statusBarH: 40
    readonly property int navBarH:    72
    readonly property int contentH:   488
    readonly property int screenW:    1024
    readonly property int screenH:    600
}
