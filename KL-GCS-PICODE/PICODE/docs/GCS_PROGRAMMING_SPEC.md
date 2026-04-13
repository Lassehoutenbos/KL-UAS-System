# GCS Pi Screen — Complete Programming Specification
**Platform:** Raspberry Pi · Qt 6.10 · Two 1024×600 7" Touchscreens  
**Language:** C++17 + QML  
**Build system:** CMake  
**Status:** Implementation reference — all decisions made, no ambiguity

---

## 1. Build Setup

### 1.1 Required Qt Modules

Update `CMakeLists.txt` — replace the current minimal setup:

```cmake
cmake_minimum_required(VERSION 3.16)
project(PICODE VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    QuickControls2
    SerialPort
    Multimedia
    MultimediaQuick
    Positioning
    Location
    Network
)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(appPICODE
    main.cpp
    backend/gcsstate.h   backend/gcsstate.cpp
    backend/picolink.h   backend/picolink.cpp
    backend/mavlinklink.h backend/mavlinklink.cpp
)

qt_add_qml_module(appPICODE
    URI PICODE
    QML_FILES
        Main.qml
        components/StatusBar.qml
        components/NavBar.qml
        components/DataCard.qml
        components/BigButton.qml
        components/SliderRow.qml
        components/StatusDot.qml
        components/ConfirmOverlay.qml
        components/QuickPanel.qml
        pages/DashPage.qml
        pages/DronePage.qml
        pages/CameraPage.qml
        pages/MapPage.qml
        pages/MissionPage.qml
        pages/ParamsPage.qml
        pages/PeriphPage.qml
        pages/CasePage.qml
    RESOURCES
        assets/fonts/Inter-Regular.ttf
        assets/fonts/Inter-SemiBold.ttf
        assets/fonts/Inter-Bold.ttf
)

target_link_libraries(appPICODE PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::SerialPort
    Qt6::Multimedia
    Qt6::MultimediaQuick
    Qt6::Positioning
    Qt6::Location
    Qt6::Network
)
```

### 1.2 Directory Layout

```
PICODE/
├── CMakeLists.txt
├── main.cpp
├── Main.qml
├── backend/
│   ├── gcsstate.h / .cpp       — singleton: all live data as Q_PROPERTYs
│   ├── picolink.h / .cpp       — USB CDC serial parser (Pico protocol)
│   └── mavlinklink.h / .cpp    — MAVLink serial/UDP parser
├── components/
│   ├── StatusBar.qml
│   ├── NavBar.qml
│   ├── DataCard.qml
│   ├── BigButton.qml
│   ├── SliderRow.qml
│   ├── StatusDot.qml
│   ├── ConfirmOverlay.qml
│   └── QuickPanel.qml
├── pages/
│   ├── DashPage.qml
│   ├── DronePage.qml
│   ├── CameraPage.qml
│   ├── MapPage.qml
│   ├── MissionPage.qml
│   ├── ParamsPage.qml
│   ├── PeriphPage.qml
│   └── CasePage.qml
└── assets/
    └── fonts/
        ├── Inter-Regular.ttf
        ├── Inter-SemiBold.ttf
        └── Inter-Bold.ttf
```

---

## 2. Theme Constants

Define a single `Theme` QML singleton (inline pragma singleton in a file `Theme.qml`, added to QML_FILES).

```qml
// Theme.qml
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

    // Typography sizes (px at 170 DPI = real pt, use sp/px in QML)
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
    readonly property int contentH:   488   // 600 - 40 - 72
    readonly property int screenW:    1024
    readonly property int screenH:    600
}
```

Register it in `main.cpp`:
```cpp
qmlRegisterSingletonType(QUrl("qrc:/qt/qml/PICODE/Theme.qml"), "PICODE", 1, 0, "Theme");
```

Import in every QML file:
```qml
import PICODE
```

---

## 3. Backend: GCSState Singleton

### 3.1 Header — `backend/gcsstate.h`

```cpp
#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QDateTime>
#include <QVariantList>

class GCSState : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    // --- MAVLink / Drone ---
    Q_PROPERTY(double  altitude        READ altitude        NOTIFY droneStateChanged)
    Q_PROPERTY(double  groundSpeed     READ groundSpeed     NOTIFY droneStateChanged)
    Q_PROPERTY(double  heading         READ heading         NOTIFY droneStateChanged)
    Q_PROPERTY(double  verticalSpeed   READ verticalSpeed   NOTIFY droneStateChanged)
    Q_PROPERTY(double  pitch           READ pitch           NOTIFY droneStateChanged)
    Q_PROPERTY(double  roll            READ roll            NOTIFY droneStateChanged)
    Q_PROPERTY(double  yaw             READ yaw             NOTIFY droneStateChanged)
    Q_PROPERTY(int     gpsSats         READ gpsSats         NOTIFY droneStateChanged)
    Q_PROPERTY(QString gpsFixType     READ gpsFixType      NOTIFY droneStateChanged)  // "NO-FIX","2D-FIX","3D-FIX"
    Q_PROPERTY(double  batteryVoltage  READ batteryVoltage  NOTIFY droneStateChanged)
    Q_PROPERTY(int     batteryPercent  READ batteryPercent  NOTIFY droneStateChanged)
    Q_PROPERTY(double  extVoltage      READ extVoltage      NOTIFY droneStateChanged)
    Q_PROPERTY(bool    mavlinkConnected READ mavlinkConnected NOTIFY linkStateChanged)
    Q_PROPERTY(bool    droneArmed      READ droneArmed      NOTIFY droneStateChanged)
    Q_PROPERTY(QString flightMode      READ flightMode      NOTIFY droneStateChanged)
    Q_PROPERTY(int     mavlinkRssi     READ mavlinkRssi     NOTIFY linkStateChanged)  // dBm
    Q_PROPERTY(int     mavlinkSnr      READ mavlinkSnr      NOTIFY linkStateChanged)  // dB
    Q_PROPERTY(QStringList statusMessages READ statusMessages NOTIFY statusMessagesChanged)

    // --- GCS Hardware (from Pico) ---
    Q_PROPERTY(bool    picoConnected   READ picoConnected   NOTIFY linkStateChanged)
    Q_PROPERTY(double  picoHeartbeatMs READ picoHeartbeatMs NOTIFY linkStateChanged)
    Q_PROPERTY(int     picoHeartbeatSeq READ picoHeartbeatSeq NOTIFY linkStateChanged)
    Q_PROPERTY(bool    keyUnlocked     READ keyUnlocked     NOTIFY keyStateChanged)
    Q_PROPERTY(double  alsLux         READ alsLux          NOTIFY sensorChanged)
    Q_PROPERTY(int     alsGain        READ alsGain         NOTIFY sensorChanged)
    Q_PROPERTY(int     alsIntMs       READ alsIntMs        NOTIFY sensorChanged)
    Q_PROPERTY(double  tempCaseA      READ tempCaseA       NOTIFY sensorChanged)  // ADC CH2
    Q_PROPERTY(double  tempCaseB      READ tempCaseB       NOTIFY sensorChanged)  // ADC CH3
    Q_PROPERTY(double  tempCaseC      READ tempCaseC       NOTIFY sensorChanged)  // ADC CH4
    Q_PROPERTY(double  tempCaseD      READ tempCaseD       NOTIFY sensorChanged)  // ADC CH5
    Q_PROPERTY(double  tempCpuPi      READ tempCpuPi       NOTIFY sensorChanged)  // /sys/class/thermal

    // --- Warnings (from Pico PROTO_TYPE_WARNING) ---
    // Each warning is an int: 0=OK, 1=WARN, 2=CRIT
    Q_PROPERTY(int warnTemp    READ warnTemp    NOTIFY warningsChanged)
    Q_PROPERTY(int warnSignal  READ warnSignal  NOTIFY warningsChanged)
    Q_PROPERTY(int warnDrone   READ warnDrone   NOTIFY warningsChanged)
    Q_PROPERTY(int warnGps     READ warnGps     NOTIFY warningsChanged)
    Q_PROPERTY(int warnLink    READ warnLink    NOTIFY warningsChanged)
    Q_PROPERTY(int warnNetwork READ warnNetwork NOTIFY warningsChanged)
    Q_PROPERTY(bool anyWarningActive READ anyWarningActive NOTIFY warningsChanged)

    // --- Brightness (sent/received via Pico PROTO_TYPE_BRIGHTNESS) ---
    Q_PROPERTY(int brightnessScreenL READ brightnessScreenL WRITE setBrightnessScreenL NOTIFY brightnessChanged)
    Q_PROPERTY(int brightnessScreenR READ brightnessScreenR WRITE setBrightnessScreenR NOTIFY brightnessChanged)
    Q_PROPERTY(int brightnessLed     READ brightnessLed     WRITE setBrightnessLed     NOTIFY brightnessChanged)
    Q_PROPERTY(int brightnessTft     READ brightnessTft     WRITE setBrightnessTft     NOTIFY brightnessChanged)
    Q_PROPERTY(int brightnessBtnLeds READ brightnessBtnLeds WRITE setBrightnessBtnLeds NOTIFY brightnessChanged)
    Q_PROPERTY(bool alsAutoEnabled   READ alsAutoEnabled    WRITE setAlsAutoEnabled    NOTIFY brightnessChanged)

    // --- Peripherals (RS-485) ---
    Q_PROPERTY(QVariantList peripherals READ peripherals NOTIFY peripheralsChanged)
    // peripherals is a list of QVariantMap, each with:
    //   "address": int, "name": QString, "online": bool, "tftActive": bool
    //   + device-specific keys per type (see §10)

    // --- System (Pi-local) ---
    Q_PROPERTY(double  uptimeSeconds  READ uptimeSeconds  NOTIFY systemChanged)
    Q_PROPERTY(int     memPercent     READ memPercent     NOTIFY systemChanged)
    Q_PROPERTY(int     diskPercent    READ diskPercent    NOTIFY systemChanged)

    // --- Mission ---
    Q_PROPERTY(QVariantList waypoints READ waypoints WRITE setWaypoints NOTIFY waypointsChanged)
    // Each QVariantMap: "index":int, "type":QString, "lat":double, "lon":double,
    //                   "altM":double, "speedMs":double, "bearing":double

    // --- Parameters ---
    Q_PROPERTY(QVariantList params READ params NOTIFY paramsChanged)
    // Each QVariantMap: "name":QString, "value":double, "min":double, "max":double,
    //                   "group":QString, "dirty":bool (changed but not written)

public:
    static GCSState *create(QQmlEngine *, QJSEngine *) {
        return instance();
    }
    static GCSState *instance();

    // Getters (all defined in .cpp, backed by private members)
    double  altitude()        const; double  groundSpeed()   const;
    double  heading()         const; double  verticalSpeed() const;
    double  pitch()           const; double  roll()          const;
    double  yaw()             const; int     gpsSats()       const;
    QString gpsFixType()     const; double  batteryVoltage() const;
    int     batteryPercent()  const; double  extVoltage()    const;
    bool    mavlinkConnected()const; bool    droneArmed()    const;
    QString flightMode()     const; int     mavlinkRssi()   const;
    int     mavlinkSnr()      const; QStringList statusMessages() const;
    bool    picoConnected()   const; double  picoHeartbeatMs()const;
    int     picoHeartbeatSeq()const; bool    keyUnlocked()   const;
    double  alsLux()          const; int     alsGain()       const;
    int     alsIntMs()        const; double  tempCaseA()     const;
    double  tempCaseB()       const; double  tempCaseC()     const;
    double  tempCaseD()       const; double  tempCpuPi()     const;
    int     warnTemp()        const; int     warnSignal()    const;
    int     warnDrone()       const; int     warnGps()       const;
    int     warnLink()        const; int     warnNetwork()   const;
    bool    anyWarningActive()const;
    int     brightnessScreenL()const; int   brightnessScreenR()const;
    int     brightnessLed()   const;  int   brightnessTft()  const;
    int     brightnessBtnLeds()const; bool  alsAutoEnabled() const;
    QVariantList peripherals()const;
    double  uptimeSeconds()   const; int   memPercent()      const;
    int     diskPercent()     const;
    QVariantList waypoints()  const;
    QVariantList params()     const;

public slots:
    // Called by PicoLink / MavlinkLink to push new data
    void updateDroneState(double alt, double spd, double hdg, double vspd,
                          double pitch, double roll, double yaw);
    void updateGps(int sats, const QString &fixType);
    void updateBattery(double voltage, int percent, double extV);
    void updateMavlinkLink(bool connected, int rssiDbm, int snrDb);
    void updateFlightMode(const QString &mode, bool armed);
    void appendStatusMessage(const QString &msg);  // keeps last 3
    void updatePicoLink(bool connected, double heartbeatMs, int seq);
    void updateKeyState(bool unlocked);
    void updateAlsSensor(double lux, int gain, int intMs);
    void updateCaseTemps(double a, double b, double c, double d);
    void updateCpuTemp(double celsius);
    void updateWarnings(int temp, int signal, int drone, int gps, int link, int network);
    void updatePeripheral(int address, const QString &name, bool online,
                          const QVariantMap &deviceData);
    void updateSystemStats(double uptimeSec, int memPct, int diskPct);
    void setWaypoints(const QVariantList &wps);
    void updateParams(const QVariantList &params);
    void setParamDirty(const QString &name, double newValue);

    // Brightness setters (also trigger Pico PROTO_TYPE_BRIGHTNESS send via PicoLink)
    void setBrightnessScreenL(int v);
    void setBrightnessScreenR(int v);
    void setBrightnessLed(int v);
    void setBrightnessTft(int v);
    void setBrightnessBtnLeds(int v);
    void setAlsAutoEnabled(bool enabled);

    // MAVLink commands (emits signals caught by MavlinkLink)
    Q_INVOKABLE void sendFlightMode(const QString &mode);
    Q_INVOKABLE void sendArmDisarm(bool arm);
    Q_INVOKABLE void sendMissionUpload();
    Q_INVOKABLE void sendMissionDownload();
    Q_INVOKABLE void sendParamWrite(const QString &name, double value);
    Q_INVOKABLE void sendParamRefresh();

signals:
    void droneStateChanged();
    void linkStateChanged();
    void keyStateChanged();
    void sensorChanged();
    void warningsChanged();
    void brightnessChanged();
    void peripheralsChanged();
    void systemChanged();
    void waypointsChanged();
    void paramsChanged();
    void statusMessagesChanged();
    // Internal command signals (caught by MavlinkLink / PicoLink)
    void cmdSetMode(const QString &mode);
    void cmdArmDisarm(bool arm);
    void cmdMissionUpload(const QVariantList &wps);
    void cmdMissionDownload();
    void cmdParamWrite(const QString &name, double value);
    void cmdParamRefresh();
    void cmdBrightnessChanged(int screenL, int screenR, int led, int tft, int btnLeds);

private:
    explicit GCSState(QObject *parent = nullptr);
    static GCSState *s_instance;
    // Private member storage (one per property, all initialized to safe defaults)
};
```

---

## 4. Backend: PicoLink

### 4.1 Responsibilities
- Opens `/dev/ttyACM0` (USB CDC) at 115200 baud via `QSerialPort`
- Parses binary frames from `protocol.h` (GCS firmware)
- Calls `GCSState` update slots on each received packet
- Sends brightness/warning/screen packets when `GCSState` emits `cmdBrightnessChanged`
- Emits heartbeat to Pico every 1000ms; tracks response to set `picoConnected`
- Reads Pi CPU temp from `/sys/class/thermal/thermal_zone0/temp` every 2000ms
- Reads system stats (`/proc/uptime`, `/proc/meminfo`, `df /`) every 5000ms

### 4.2 Packet handling table

| Received `proto_type` | Action |
|---|---|
| `PROTO_TYPE_HEARTBEAT` | `gcsState->updatePicoLink(true, elapsed, seq)` |
| `PROTO_TYPE_ADC` | Parse CH0=battery, CH2–CH5=NTC → `updateBattery()` + `updateCaseTemps()` |
| `PROTO_TYPE_DIGITAL` | Parse key switch bit → `updateKeyState()` |
| `PROTO_TYPE_WARNING` | Parse 9 warning levels → `updateWarnings()` |
| `PROTO_TYPE_ALS` | Parse lux/gain/int → `updateAlsSensor()` |
| `PROTO_TYPE_EVENT` | Parse event ID → forward to GCSState as appropriate |
| `PROTO_TYPE_PERIPH_RESPONSE` | Parse address + payload → `updatePeripheral()` |

| To send | Trigger |
|---|---|
| `PROTO_TYPE_BRIGHTNESS` | `GCSState::cmdBrightnessChanged` signal |
| `PROTO_TYPE_WARNING` | Called by MavlinkLink when drone health changes |
| `PROTO_TYPE_SCREEN` | Called when TFT mode logic determines mode change |
| `PROTO_TYPE_PERIPH_CMD` | Called from QML via `GCSState::sendPeriphCmd()` |

### 4.3 Reconnect logic
If `QSerialPort` emits `errorOccurred`, close and retry every 2000ms. Set `picoConnected = false` while disconnected.

---

## 5. Backend: MavlinkLink

### 5.1 Transport
- Default: serial `/dev/ttyUSB0` at 57600 baud
- Fallback: UDP port 14550 (for SITL / ground testing)
- Auto-detect: try serial first; if no heartbeat in 5s, switch to UDP
- Use `QSerialPort` for serial, `QUdpSocket` for UDP

### 5.2 MAVLink messages to parse

| MAVLink message | Fields used | GCSState call |
|---|---|---|
| `HEARTBEAT` | `custom_mode`, `base_mode` (ARMED flag) | `updateFlightMode()`, `updateMavlinkLink(true,...)` |
| `VFR_HUD` | `alt`, `groundspeed`, `heading`, `climb` | `updateDroneState(alt, spd, hdg, vspd, ...)` |
| `ATTITUDE` | `pitch`, `roll`, `yaw` | `updateDroneState(..., pitch, roll, yaw)` |
| `GPS_RAW_INT` | `satellites_visible`, `fix_type` | `updateGps()` |
| `SYS_STATUS` | `voltage_battery`, `battery_remaining` | `updateBattery()` |
| `RADIO_STATUS` | `rssi`, `noise` | `updateMavlinkLink(true, rssi, snr)` |
| `STATUSTEXT` | `text` | `appendStatusMessage()` |
| `PARAM_VALUE` | `param_id`, `param_value`, `param_count` | `updateParams()` (buffer until all received) |
| `MISSION_COUNT` | triggers download sequence | internal |
| `MISSION_ITEM_INT` | all fields | `setWaypoints()` when last item received |
| `GLOBAL_POSITION_INT` | `lat`, `lon`, `alt` | update drone lat/lon in GCSState (add these properties) |

### 5.3 MAVLink commands to send

| `GCSState` signal | MAVLink message to send |
|---|---|
| `cmdSetMode(mode)` | `COMMAND_LONG` MAV_CMD_DO_SET_MODE |
| `cmdArmDisarm(arm)` | `COMMAND_LONG` MAV_CMD_COMPONENT_ARM_DISARM |
| `cmdMissionUpload(wps)` | `MISSION_COUNT` → `MISSION_REQUEST_INT` → `MISSION_ITEM_INT` |
| `cmdMissionDownload()` | `MISSION_REQUEST_LIST` → receive `MISSION_ITEM_INT` × n |
| `cmdParamWrite(name, val)` | `PARAM_SET` |
| `cmdParamRefresh()` | `PARAM_REQUEST_LIST` |

### 5.4 Heartbeat timeout
If no `HEARTBEAT` received for 3000ms, call `updateMavlinkLink(false, 0, 0)`.

---

## 6. main.cpp — Dual-Window Setup

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include "backend/gcsstate.h"
#include "backend/picolink.h"
#include "backend/mavlinklink.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Register singleton
    qmlRegisterSingletonInstance("PICODE", 1, 0, "GCSState", GCSState::instance());

    // Start backends
    PicoLink  *pico    = new PicoLink(GCSState::instance(), &app);
    MavlinkLink *mav   = new MavlinkLink(GCSState::instance(), &app);
    pico->start();
    mav->start();

    QList<QScreen *> screens = app.screens();

    // Load Screen Left (index 0) — default page: DASH
    QQmlApplicationEngine engine0;
    engine0.rootContext()->setContextProperty("screenIndex", 0);
    engine0.rootContext()->setContextProperty("initialPage", "dash");
    engine0.loadFromModule("PICODE", "Main");

    // Load Screen Right (index 1) — default page: DRONE
    // Only create second window if a second screen is connected
    QQmlApplicationEngine *engine1 = nullptr;
    if (screens.size() >= 2) {
        engine1 = new QQmlApplicationEngine(&app);
        engine1->rootContext()->setContextProperty("screenIndex", 1);
        engine1->rootContext()->setContextProperty("initialPage", "drone");
        engine1->loadFromModule("PICODE", "Main");
    }

    return app.exec();
}
```

---

## 7. Main.qml — Root Window

```
Responsibilities:
- Full-screen window on the assigned screen
- Holds currentPage string state (navigation)
- Renders: StatusBar (top) + Loader (content) + NavBar (bottom)
- Renders QuickPanel overlay (swipe-down from status bar)
- Passes screenIndex to children that need it
```

### 7.1 Window setup

```qml
Window {
    id: root
    property int  screenIndex: 0       // injected from C++
    property string initialPage: "dash" // injected from C++
    property string currentPage: initialPage

    // Position on the correct physical screen
    screen: Qt.application.screens[screenIndex] ?? Qt.application.screens[0]
    x: screen.virtualX
    y: screen.virtualY
    width:  1024
    height: 600
    visibility: Window.FullScreen
    color: Theme.bgPrimary
    ...
}
```

### 7.2 Layout structure (z-ordering)

```
z=0  Rectangle { color: Theme.bgPrimary }  // background
z=1  StatusBar  { id: statusBar; anchors.top }
z=1  Loader     { id: pageLoader; between statusBar and navBar }
z=1  NavBar     { id: navBar;    anchors.bottom }
z=10 QuickPanel { id: quickPanel; anchors.top; visible: false }
z=20 InputPanel { id: inputPanel; VirtualKeyboard }
```

### 7.3 Page routing

```qml
Loader {
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
            case "periph":  return "pages/PeriphPage.qml"
            case "case":    return "pages/CasePage.qml"
            default:        return "pages/DashPage.qml"
        }
    }
}
```

---

## 8. Components

### 8.1 StatusBar.qml

**Size:** 1024 × 40px  
**Background:** `Theme.bgSecondary`  
**Bottom border:** 1px `Theme.border`

Segments (left to right), separated by `│` (`Rectangle { width:1; color: Theme.border }`):

| Segment | Width | Content | Binding |
|---|---|---|---|
| Link | 110px | `● CONNECTED` or `○ NO LINK` | `GCSState.mavlinkConnected` |
| GPS | 140px | `GPS 14  3D-FIX` | `GCSState.gpsSats` + `GCSState.gpsFixType` |
| Battery | 160px | `BAT 22.4V ████░` | `GCSState.batteryVoltage` + `GCSState.batteryPercent` |
| Altitude | 100px | `ALT 045m` | `GCSState.altitude` |
| Clock | 80px | `HH:MM` | `Qt.formatTime(new Date(), "hh:mm")` — update every 30s |

Battery bar: 5-segment row of small rectangles (24×8px each), filled count = `Math.round(batteryPercent / 20)`, filled color = `Theme.statusOk` (<20% → `Theme.statusCrit`), unfilled = `Theme.bgElevated`.

Text: all caps, 11px, weight 500. Link dot: `StatusDot` component. Tap anywhere on status bar triggers `quickPanel.open()`.

### 8.2 NavBar.qml

**Size:** 1024 × 72px  
**Background:** `Theme.bgSecondary`  
**Top border:** 1px `Theme.border`

8 tabs × 128px wide:

| Index | Label | Page key |
|---|---|---|
| 0 | DASH | "dash" |
| 1 | DRONE | "drone" |
| 2 | CAMERA | "camera" |
| 3 | MAP | "map" |
| 4 | MISSION | "mission" |
| 5 | PARAMS | "params" |
| 6 | PERIPH | "periph" |
| 7 | CASE | "case" |

Each tab: `Rectangle` 128×72px. On active (`root.currentPage === pageKey`):
- Bottom border: 4px `Theme.accentYellow`  
- Label color: `Theme.accentYellow`

On inactive:
- No bottom border
- Label color: `Theme.textSecondary`

Label: text 13px weight 600, uppercase. No icons needed for v1.

Signal: `onTabClicked(pageKey)` → parent sets `root.currentPage = pageKey`.

### 8.3 DataCard.qml

**Properties:** `label: string`, `value: string`, `unit: string`  
**Size:** 156 × 100px  
**Background:** `Theme.bgSecondary`  
**Border:** 1px `Theme.border`, radius 4px  
**Layout (top→bottom):**
- Label: top-left, 11px, `Theme.textSecondary`, uppercase
- Value: center, 32px bold, `Theme.textPrimary`
- Unit: bottom-right, 11px, `Theme.textSecondary`

### 8.4 BigButton.qml

**Properties:** `label: string`, `active: bool = false`, `enabled: bool = true`, `danger: bool = false`  
**Default size:** 220 × 72px  
**Background:**
- `active` = true → `Theme.accentYellow`, label `Theme.bgPrimary`
- `danger` = true and `active` → `Theme.statusCrit`, label `Theme.textPrimary`
- normal idle → `Theme.bgElevated`, label `Theme.textPrimary`
- disabled → `Theme.bgSecondary`, label `Theme.textDisabled`

**Border:** 1px `Theme.border`, radius 6px  
**Label:** 13px, weight 600, uppercase, centered  
**Signal:** `onClicked()`

### 8.5 SliderRow.qml

**Properties:** `label: string`, `value: int` (0–100), `unit: string = "%"`, `readOnly: bool = false`  
**Size:** full width × 48px  
**Layout:** Label left (120px), Slider fills remaining space, value label right (60px)  
**Slider:** `QtQuick.Controls.Slider`, `from: 0`, `to: 100`, `stepSize: 1`  
- Track height 6px, `Theme.bgElevated` unfilled, `Theme.accentYellow` filled
- Handle: 28×28px circle, `Theme.accentYellow`, readonly handle = `Theme.textDisabled`  
**Signal:** `onValueChanged(int v)`

### 8.6 StatusDot.qml

**Properties:** `level: int` (0=OK, 1=WARN, 2=CRIT, -1=unknown)  
**Size:** 10 × 10px, circle  
**Colors:** 0→`Theme.statusOk`, 1→`Theme.statusWarn`, 2→`Theme.statusCrit`, -1→`Theme.textDisabled`

### 8.7 ConfirmOverlay.qml

**Properties:** `title: string`, `body: string`, `confirmLabel: string`, `destructive: bool`  
**Trigger:** `open()` / `close()` methods; `signal confirmed()`; `signal cancelled()`

**Layout:** Modal Rectangle centered in parent, 440 × 220px, `Theme.bgSecondary`, border `Theme.border` 1px, radius 8px.
- Scrim behind: full parent, `Qt.rgba(0,0,0,0.6)`
- Title: 16px, weight 600, `Theme.textPrimary`
- Body: 13px, `Theme.textSecondary`, wraps
- CANCEL button: 180×64px, `Theme.bgElevated`
- CONFIRM button: 180×64px, `destructive` = `Theme.statusCrit`, else `Theme.statusOk`

### 8.8 QuickPanel.qml

See §11 for full spec.

---

## 9. Pages

All pages: `Rectangle { color: Theme.bgPrimary; width: 1024; height: 488 }`

### 9.1 DashPage.qml

**Three-column layout:** Left 280px | Center 464px | Right 280px

#### Left column (280px) — Drone stats
Rows of `label: value` pairs, 40px tall each. Section headers: thin `Rectangle` divider + label 11px `Theme.textSecondary`. Values: 20px, weight 600, `Theme.textPrimary`.

```
Section: FLIGHT
  ALT     GCSState.altitude.toFixed(1) + " m"
  SPEED   GCSState.groundSpeed.toFixed(1) + " m/s"
  HDG     GCSState.heading.toFixed(0) + "°"
  VSPD    (GCSState.verticalSpeed >= 0 ? "+" : "") + GCSState.verticalSpeed.toFixed(1) + " m/s"

Section: STATUS
  MODE    GCSState.flightMode
  ARMED   StatusDot (level: GCSState.droneArmed ? 2 : 0) + (armed ? "ARMED" : "SAFE")

Section: POWER
  BAT     GCSState.batteryVoltage.toFixed(1) + "V  " + GCSState.batteryPercent + "%"
  EXT     GCSState.extVoltage.toFixed(1) + "V"
```

#### Center column (464px) — Attitude Indicator (AHI)

Use a custom `Canvas` item (480×280px, vertically centered).

**AHI drawing (Canvas `onPaint`):**
1. **Sky/ground split:** Draw filled rectangle. Sky = `#1a3a5c`, Ground = `#3d2b1a`. Split at center + `pitch * pixelsPerDegree`.
2. **Pitch ladder:** Lines every 5°, labeled every 10°. Color `Theme.textPrimary`, opacity 0.6. `ctx.rotate(-roll * Math.PI/180)` before drawing.
3. **Roll arc:** Semicircle at top (radius 100px). Tick marks every 10°. Pointer triangle at center top.
4. **Fixed aircraft symbol:** Two horizontal lines from center (yellow, 3px thick).
5. **Heading tape:** Strip along bottom 30px. Tick every 10°, label every 30°. Drone heading `GCSState.heading` centered.

Redraw on: `GCSState.droneStateChanged`

Below AHI canvas, three data items in one row:
- `PITCH  +2.1°`  |  `ROLL  -0.8°`  |  `YAW  214°`

Font 14px, `Theme.textPrimary`.

#### Right column (280px) — GCS state

```
Section: GCS
  LINK    StatusDot(picoConnected) + (picoConnected ? "OK" : "NO LINK")
  GPS     GCSState.gpsSats + "  " + GCSState.gpsFixType
  ALS     GCSState.alsLux.toFixed(0) + " lux"
  TEMP    GCSState.tempCpuPi.toFixed(0) + "°C"

Section: WARNINGS
  TEMP    StatusDot(warnTemp)  + "TEMP"
  SIG     StatusDot(warnSignal)+ "SIGNAL"
  DRONE   StatusDot(warnDrone) + "DRONE"
  GPS     StatusDot(warnGps)   + "GPS"
  LINK    StatusDot(warnLink)  + "LINK"
  NET     StatusDot(warnNetwork)+"NET"
```

---

### 9.2 DronePage.qml

**Two-column layout:** Left 520px | Right 504px

#### Left — Telemetry cards (2-column grid)

6 × `DataCard` in a `GridView` (2 columns, cell 250×110px, 10px spacing):

| Card | Value | Unit |
|---|---|---|
| ALT | `GCSState.altitude.toFixed(1)` | m |
| SPEED | `GCSState.groundSpeed.toFixed(1)` | m/s |
| HDG | `GCSState.heading.toFixed(0)` | ° |
| VSPD | `GCSState.verticalSpeed.toFixed(1)` | m/s |
| SAT | `GCSState.gpsSats` | — |
| HDOP | from MAVLink GPS2_RAW (add `hdop: double` to GCSState) | — |

Below the grid, battery bar card full-width:
```
Rectangle 500×56px bgSecondary border
  "BATTERY" label left
  Voltage + percent right
  Progress bar (full width, 12px tall) below text
    filled = batteryPercent/100 × bar width
    color: percent > 30 → statusOk, 10-30 → statusWarn, <10 → statusCrit
```

#### Right — Control

**FLIGHT MODE grid (3×2):** 6 × `BigButton` 156×72px, 8px gaps.

| Row 0 | STABILIZE | LOITER | AUTO |
|---|---|---|---|
| Row 1 | ALTHOLD | RTL | GUIDED |

`active: GCSState.flightMode === buttonMode`

On tap → `ConfirmOverlay { title: "SET MODE"; body: "Switch to " + mode + "?" }` → on confirmed → `GCSState.sendFlightMode(mode)`

**ARM / DISARM button:** Full width 72px.
- Disarmed state: label "ARM DRONE", background `Theme.bgElevated`
- Armed state: label "DISARM", background `Theme.statusCrit`
- If `!GCSState.keyUnlocked`: overlay text "LOCKED", button disabled, opacity 0.4
- On tap → `ConfirmOverlay { title: "ARM DRONE", body: "This will arm motors.", destructive: true }` → `GCSState.sendArmDisarm(true)`

**System message log:** `ListView` 500×80px, last 3 messages from `GCSState.statusMessages`. Each row: `>` prefix, 12px `Theme.textSecondary`.

---

### 9.3 CameraPage.qml

**Video area:** 1024 × 420px

```qml
import QtMultimedia

MediaPlayer {
    id: player
    source: "gst-pipeline: v4l2src device=/dev/video0 ! videoconvert ! qtvideosink"
    // fallback: "v4l2:///dev/video0"
    autoPlay: true
}

VideoOutput {
    anchors.fill: videoArea
    source: player    // Qt6: use VideoSink pattern — see note below
}
```

> **Qt6 note:** In Qt 6, use `VideoOutput { id: vo }` and `player.videoOutput = vo`. The `source` property is removed in Qt6 Multimedia.

**HUD overlay items** (all `z: 10`, anchored within `videoArea`):

| Position | Content | Binding |
|---|---|---|
| top-left + 8px | `ALT 045m` | `GCSState.altitude.toFixed(0)` |
| top-right - 8px | `HDG 214°` | `GCSState.heading.toFixed(0)` |
| bottom-left + 8px | `SPD 12.4m/s` | `GCSState.groundSpeed.toFixed(1)` |
| bottom-right - 8px | `BAT 22.4V` | `GCSState.batteryVoltage.toFixed(1)` |

Each HUD label: `Rectangle { color: Qt.rgba(0,0,0,0.35); radius: 4 }` behind text.

HUD visibility: `visible: hudOverlayEnabled` (local bool property, toggled by quick panel).

**Bottom controls strip:** 68px, `Theme.bgSecondary`.
- Zoom slider: `SliderRow` (0–100, label "ZOOM"), takes 400px
- Record button: `BigButton` 120×56px, `active: recording`, label "REC" + timer text when active
- Snapshot button: `BigButton` 100×56px, label "SNAP"
- Overlay toggle: `BigButton` 130×56px, `active: hudOverlayEnabled`, label "HUD ON/OFF"

Recording is local (Pi storage). Timer: `Timer { interval: 1000; running: recording }`.

---

### 9.4 MapPage.qml

**Left sidebar:** 220px  
**Map:** 804 × 488px

#### Sidebar
```
Label "WAYPOINTS × N"  (N = GCSState.waypoints.length)
─────────────────────
ListView (scrollable, fills sidebar minus buttons at bottom)
  Each row:
    "WP1  TAKEOFF"  (index + type, 13px textPrimary)
    "450m  214°"    (distM + bearing, 11px textSecondary)

─────────────────────
"DIST  1.24 km"  (total mission distance)
"ETE   04:12"    (estimated time en route at current speed)

BigButton 220×56px "CENTER DRONE"   → map.center = droneCoord
BigButton 220×56px "ZOOM IN"        → map.zoomLevel++
BigButton 220×56px "ZOOM OUT"       → map.zoomLevel--
```

ETE calculation: `totalDistM / GCSState.groundSpeed / 60` → format mm:ss.

#### Map widget

```qml
import QtLocation
import QtPositioning

Plugin {
    id: osmPlugin
    name: "osm"
    PluginParameter { name: "osm.mapping.offline.directory"; value: "/home/pi/maps" }
    PluginParameter { name: "osm.useragent"; value: "GCS-Pi/1.0" }
}

Map {
    id: map
    plugin: osmPlugin
    center: QtPositioning.coordinate(52.0, 5.0)  // default: Netherlands
    zoomLevel: 14
    width: 804; height: 488

    // Drone marker
    MapQuickItem {
        coordinate: QtPositioning.coordinate(GCSState.droneLat, GCSState.droneLon)
        anchorPoint: Qt.point(12, 12)
        sourceItem: Canvas {
            width: 24; height: 24
            onPaint: { /* draw yellow chevron rotated to GCSState.heading */ }
        }
    }

    // Waypoints
    MapItemView {
        model: GCSState.waypoints
        delegate: MapQuickItem {
            coordinate: QtPositioning.coordinate(modelData.lat, modelData.lon)
            sourceItem: Rectangle {
                width: 24; height: 24; radius: 12
                color: Theme.accentBlue
                Text { text: modelData.index; color: "white"; anchors.centerIn: parent }
            }
        }
    }

    // Route line
    MapPolyline {
        line.color: Theme.accentBlue
        line.width: 2
        path: GCSState.waypoints.map(wp => QtPositioning.coordinate(wp.lat, wp.lon))
    }
}
```

Add `droneLat: double` and `droneLon: double` to `GCSState` (updated from `GLOBAL_POSITION_INT`).

Offline: when no network, the OSM plugin automatically uses the cache/offline directory.

---

### 9.5 MissionPage.qml

**Map (editable):** 640px  
**Editor panel:** 384px

Map: same as MapPage but `gesture.enabled: true`. On tap on empty space → add waypoint at tapped coordinate. On tap on existing waypoint → select it (highlight `Theme.accentYellow`). Drag existing waypoint → update lat/lon.

#### Editor panel

```
Title "MISSION EDITOR"
─────────────────────
ListView (scrollable)
  Each row (height 56px, full width, border-bottom Theme.border):
    "WP1  TAKEOFF  15m"    (13px textPrimary)
    Tap to expand inline edit:
      ALT: NumericField (keypad overlay on tap)
      SPEED: NumericField
      ACTION: dropdown (TAKEOFF / WAYPOINT / LOITER / RTL / LAND)

BigButton 184×56px "＋ ADD WP"    → append WP at last known drone position
BigButton 184×56px "－ DEL WP"    → remove selected WP (disabled if none selected)

─────────────────────
BigButton full×72px "UPLOAD TO DRONE"
  → ConfirmOverlay → GCSState.sendMissionUpload()
BigButton full×72px "DOWNLOAD FROM DRONE"
  → GCSState.sendMissionDownload()
BigButton full×72px "CLEAR MISSION"
  → ConfirmOverlay { destructive: true } → GCSState.setWaypoints([])
```

---

### 9.6 ParamsPage.qml

**Left sidebar:** 220px  
**Params list:** 804px

#### Sidebar
```
TextInput (height 44px) — search filter
  Bound to filterText property; filters param list in real time

─────────────────────
ListView — parameter groups
  ATTITUDE · TUNING · BATTERY · GPS · COMPASS · FAILSAFE · RADIO · MOTORS
  Each row height 44px, active in accentYellow, inactive textSecondary
  Tap → set activeGroup; list filters to that group
```

#### Params list (filtered by activeGroup AND filterText)

```qml
ListView {
    model: filteredParams  // JS filter on GCSState.params
    delegate: Rectangle {
        height: 56; width: parent.width
        // Left: param name 13px textPrimary
        // Center: current value 14px textPrimary
        //   if dirty (changed not written): color statusWarn
        // Right: two buttons 64×56px: "−" and "+"
        //   step = 10% of (max - min), clamped to range
        //   on hold: repeat every 200ms
        // Tap anywhere on row → ParameterEditOverlay
    }
}
```

#### ParameterEditOverlay (modal, same structure as ConfirmOverlay)

```
Title: param name
Subtitle: param description (from a local lookup table Map<String,String> in C++)
Current: value   Range: min – max
─────────────────────
TextInput (numeric, pre-filled with current value)
Numeric keypad: [1][2][3][.][←]
                [4][5][6]
                [7][8][9]
                [0]
SET button: disabled if value outside range

CANCEL → close overlay
SET → GCSState.setParamDirty(name, newValue) → close overlay
```

Bottom action bar (full width, 64px):
```
BigButton half "WRITE TO DRONE"   → ConfirmOverlay → GCSState.sendParamWrite for each dirty param
BigButton half "REFRESH FROM DRONE" → GCSState.sendParamRefresh()
```

---

### 9.7 PeriphPage.qml

**Left sidebar:** 260px  
**Detail panel:** 764px

#### Sidebar

```
"DEVICES  4/4"  (online count / total)
─────────────────────
ListView:
  Each row 52px:
    StatusDot (online) + "0x01  SEARCHLIGHT"
    Selected row: border 2px accentYellow
    Offline rows: opacity 0.5
    Tag "[TFT]" in accentBlue if that device is currently on TFT detail screen

BigButton full×56px "VIEW ON TFT"
  → disabled if no device selected or selected offline
  → sends PROTO_TYPE_PERIPH_SCREEN with selected address
  → sets tftActive tag on that device, clears others

BigButton full×56px "TFT: OVERVIEW"
  → sends PROTO_TYPE_SCREEN mode 5
  → clears all tftActive tags
```

#### Detail panel (right, 764px)

Renders different sub-layouts per `peripheral.deviceType`:

**SEARCHLIGHT (deviceType: "searchlight")**
```
Title + online dot + address

BRIGHTNESS
SliderRow (0–100%) bound to peripheral.brightness
  onChange: debounce 200ms → send PROTO_TYPE_PERIPH_CMD SET_BRIGHTNESS

TEMPERATURE  42°C  StatusDot(tempLevel)
FAULTS       peripheral.faults or "NONE"

[STREAM ON 100ms]  [GET STATUS]
[SET PARAM...]     [PING]
```

**RADAR (deviceType: "radar")**
```
DISTANCE  large DataCard
SIGNAL STRENGTH  progress bar (0–100%)
STATUS  text
[GET STATUS]  [PING]
```

**PAN-TILT (deviceType: "pantilt")**
```
PAN   SliderRow (0–360°)
TILT  SliderRow (-90–+90°)
POSITION READBACK  pan: X°  tilt: Y°
[GET STATUS]  [PING]
```

**LIGHT BAR (deviceType: "lightbar")**
```
BRIGHTNESS  SliderRow (0–100%)
MODE  three BigButtons: SOLID / FLASH / BREATHE
COLOUR  color picker (hue slider + brightness) — send as RGB
[GET STATUS]  [PING]
```

Command buttons (bottom of detail panel, 180×56px each):
- STREAM ON → sends `SET_STREAM 100` → toggles to "STREAM OFF"
- GET STATUS → sends `GET_STATUS`
- SET PARAM → opens parameter input overlay (label + numeric input + SET)
- PING → sends `PING`, awaits response, shows round-trip time

---

### 9.8 CasePage.qml

#### BRIGHTNESS section

```
MASTER  SliderRow (0–100%)
  onChange → scale all other brightness values proportionally
  sends PROTO_TYPE_BRIGHTNESS (all channels)

SCREENS     SliderRow  [LINK ■] toggle button right
  LINK=true: both screens track same value
  LINK=false: "SCREEN R" second slider appears

LED STRIP   SliderRow
TFT BL      SliderRow
BTN LEDS    SliderRow

AMBIENT LIGHT AUTO-BRIGHTNESS  [ENABLED ▶] toggle BigButton
  Enabled: MASTER slider grayed (read-only, shows ALS-computed level)
  onChange → GCSState.setAlsAutoEnabled(v)
```

#### TEMPERATURES section

Two-column grid:
```
PI CPU        GCSState.tempCpuPi.toFixed(0) + "°C"    StatusDot
CASE CH2      GCSState.tempCaseA.toFixed(0) + "°C"    StatusDot
CASE CH3      GCSState.tempCaseB.toFixed(0) + "°C"    StatusDot
CASE CH4      GCSState.tempCaseC — if NaN → "--"       StatusDot level=-1
SEARCHLIGHT   from peripherals[0x01].temp if online    StatusDot
CASE CH5      GCSState.tempCaseD — if NaN → "--"
```

Temperature dot levels: <60°C → 0(OK), 60–80°C → 1(WARN), >80°C → 2(CRIT)

#### TFT SCREEN MODE section

Read-only status label showing current active TFT mode.  
No manual buttons — mode is driven by Pi logic automatically:

```
Priority order (highest wins):
  GCSState.batteryPercent < 15     → mode 4 BAT_WARNING    (PROTO_TYPE_SCREEN value=4)
  GCSState.anyWarningActive        → mode 2 WARNING         (PROTO_TYPE_SCREEN value=2)
  !GCSState.keyUnlocked            → mode 3 LOCK            (PROTO_TYPE_SCREEN value=3)
  peripherals.any(p => p.online)   → mode 5 PERIPH          (PROTO_TYPE_SCREEN value=5, cycles)
  default                          → mode 1 MAIN            (PROTO_TYPE_SCREEN value=1)
```

This logic runs in `PicoLink` on each state change — send packet only if mode changed.

#### INDICATOR LEDs section (display only, no controls)

```
LED2  ● MAVLink connected
LED3  ● Drone armed
LED4  ● Any warning active (blinks BLINK_SLOW when on)
```

These are driven by PicoLink automatically; shown here as status only.

#### SWITCH ASSIGNMENTS section

Two-column grid. Each row: switch label left, function picker right (dropdown, 200px):

```
SW1[0]  → function selector
SW1[1]  → function selector
SW1[2]  → function selector
SW2[0]  → function selector
SW2[1]  → function selector
SW2[2]  → function selector
RGB BTN 0 → function selector
RGB BTN 1 → function selector
```

Available functions:
```
NONE
TOGGLE READING LIGHT    → toggle peripheral 0x04 (light bar) ON/OFF
TOGGLE HUD OVERLAY      → toggle camera HUD
NEXT TFT MODE           → cycle TFT mode +1
MAP SOURCE ONLINE/OFFLINE
NAVIGATE → DASH
NAVIGATE → DRONE
NAVIGATE → CAMERA
NAVIGATE → MAP
```

Assignments persisted to `/home/pi/.config/gcs/switch_assignments.json` (read on startup, written on change). `PicoLink` dispatches incoming `EVT_SWITCH_CHANGED` / `EVT_BUTTON_PRESSED` events through `GCSState` which emits `switchTriggered(int switchId)` → `Main.qml` handles the assigned function.

---

## 10. Quick Panel

**Triggered by:** tap on status bar → `quickPanel.open()`  
**Animation:** `NumberAnimation` on `y`, 150ms `Easing.OutCubic`. Slides from `y: -280` to `y: 0`.  
**Dismiss:** swipe up, tap scrim, or `×` button.  
**Per-screen:** each `Main.qml` has its own `QuickPanel` instance. No cross-screen coupling.

### Layout (full width × 280px)

**Background:** `Theme.bgSecondary`, bottom border 1px `Theme.border`  
**Scrim below panel:** `Rectangle { color: Qt.rgba(0,0,0,0.45); y: 280; height: parent.height - 280 }`

#### Row 1: Quick Toggles (height 80px)

4 × `BigButton` 240×72px, 8px gaps:

| Label A | Label B | Action | Binding |
|---|---|---|---|
| `MAP: ONLINE` | `MAP: OFFLINE` | Toggle map tile source | local mapOnline bool |
| `ALS: ENABLED` | `ALS: DISABLED` | `GCSState.setAlsAutoEnabled()` | `GCSState.alsAutoEnabled` |
| `READING: ON` | `READING: OFF` | Toggle peripheral 0x04 | peripheral online state |
| `HUD: ON` | `HUD: OFF` | Toggle CameraPage HUD | local hudEnabled bool |

Active state: `Theme.accentYellow` border.

#### Row 2: System (height 56px)

Two rows of inline label:value pairs, 12px, `Theme.textSecondary` labels, `Theme.textPrimary` values:
```
PI CPU  52°C  ●  │  UPTIME  01:24:38  │  MEM  34%
CASE A  38°C  ●  │  DISK    12%       │  PICO  ● CONNECTED
```

#### Row 3: Link Quality (height 72px)

```
MAVLINK  [20-segment bar]  RSSI -72 dBm  SNR 18 dB
PICO CDC [20-segment bar]  HB 1000ms seq 124
ALS  840 lux  GAIN AUTO  INT 100ms
```

20-segment bar: `Row` of 20 × `Rectangle` 14×8px, 2px gap.  
Filled count = proportional to signal quality (0–100%). Color `Theme.accentBlue`.

---

## 11. Hardware Events

`PicoLink` receives `PROTO_TYPE_EVENT` packets and calls `GCSState.handleEvent(id, value)`.  
`GCSState` emits `switchTriggered(int switchId, int value)`.  
`Main.qml` connects to this signal and executes the assigned function from switch assignments.

```
EVT_KEY_LOCK_CHANGED   value=0 → updateKeyState(false)
                        value=1 → updateKeyState(true)
EVT_SWITCH_CHANGED     PB0/1/2 (SW1) → switchTriggered(0..2, value)
EVT_BUTTON_PRESSED     SW2[0..2]     → switchTriggered(3..5, 1)
EVT_BUTTON_PRESSED btn=0 RGB0       → switchTriggered(6, 1)
EVT_BUTTON_PRESSED btn=1 RGB1       → switchTriggered(7, 1)
```

---

## 12. GCSState — Missing Properties to Add

These are referenced in pages above but not listed in §3. Add them:

```cpp
Q_PROPERTY(double droneLat  READ droneLat  NOTIFY dronePositionChanged)
Q_PROPERTY(double droneLon  READ droneLon  NOTIFY dronePositionChanged)
Q_PROPERTY(double hdop      READ hdop      NOTIFY droneStateChanged)
signals:
    void dronePositionChanged();
    void switchTriggered(int switchId, int value);
```

---

## 13. Implementation Order

Follow this order — each step produces something visible/testable.

| Step | Task | Result |
|---|---|---|
| 1 | `GCSState` singleton with hardcoded dummy values | Bindable data model |
| 2 | `Theme.qml`, `StatusBar.qml`, `NavBar.qml`, page routing in `Main.qml` | Navigation skeleton |
| 3 | `PicoLink` CDC parser → populates GCSState from real hardware | Live GCS data |
| 4 | `CasePage.qml` — brightness + temperatures | Immediate hardware value |
| 5 | `DashPage.qml` — AHI canvas + left/right panels | Main operator view |
| 6 | `MavlinkLink` — heartbeat + VFR_HUD + ATTITUDE | Drone telemetry live |
| 7 | `DronePage.qml` — telemetry cards + mode buttons + ARM | Drone control |
| 8 | `PeriphPage.qml` — RS-485 peripherals | Peripheral control |
| 9 | `CameraPage.qml` — video feed + HUD overlay | Camera view |
| 10 | `MapPage.qml` — live drone position on map | Situational awareness |
| 11 | `MissionPage.qml` — editable waypoints + upload/download | Mission planning |
| 12 | `ParamsPage.qml` — param read/write | Tuning |
| 13 | `QuickPanel.qml` | Convenience overlay |
| 14 | Dual-window setup in `main.cpp` | Two physical screens |
| 15 | Switch assignments persistence + event dispatch | Hardware buttons work |
