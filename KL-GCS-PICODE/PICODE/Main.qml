import QtQuick
import PICODE

Window {
    id: root
    property int  screenIndex: 0
    property string initialPage: "dash"
    property string currentPage: initialPage

    screen: Qt.application.screens[screenIndex] ?? Qt.application.screens[0]
    x: screen.virtualX
    y: screen.virtualY
    width:  1024
    height: 600
    visible: true
    color: Theme.bgPrimary

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary
    }

    StatusBar {
        id: statusBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: Theme.statusBarH
        onQuickPanelToggled: quickPanel.toggle()
    }

    Loader {
        id: pageLoader
        anchors {
            top: statusBar.bottom
            bottom: navBar.top
            left: parent.left
            right: parent.right
        }
        source: {
            switch (root.currentPage) {
                case "dash":    return "pages/DashPage.qml"
                case "drone":   return "pages/DronePage.qml"
                case "camera":  return "pages/CameraPage.qml"
                case "map":     return "pages/MapPage.qml"
                case "mission": return "pages/MissionPage.qml"
                case "params":  return "pages/ParamsPage.qml"
                case "payload": return "pages/PayloadPage.qml"
                case "periph":  return "pages/PeriphPage.qml"
                case "case":    return "pages/CasePage.qml"
                default:        return "pages/DashPage.qml"
            }
        }
    }

    NavBar {
        id: navBar
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: Theme.navBarH
        currentPage: root.currentPage
        onTabClicked: (pageKey) => root.currentPage = pageKey
    }

    QuickPanel {
        id: quickPanel
        anchors.fill: parent
        visible: false
        z: 10
    }

    ConfirmOverlay {
        id: hwArmConfirm
        title: "ARM DRONE"
        body: "Physical ARM switch activated. Arming will enable motors. Ensure the area is clear."
        confirmLabel: "ARM"
        destructive: true
        onConfirmed: GCSState.sendArmDisarm(true)
    }

    Connections {
        target: GCSState
        function onArmSwitchToggled(on) {
            if (on && GCSState.keyUnlocked && !GCSState.droneArmed)
                hwArmConfirm.open()
        }
    }

}
