#pragma once
#include <QObject>
#include <QColor>
#include <QDateTime>
#include <QVariantList>
#include <QStringList>

class GCSState : public QObject {
    Q_OBJECT

    // --- MAVLink / Drone ---
    Q_PROPERTY(double  altitude        READ altitude        NOTIFY droneStateChanged)
    Q_PROPERTY(double  groundSpeed     READ groundSpeed     NOTIFY droneStateChanged)
    Q_PROPERTY(double  heading         READ heading         NOTIFY droneStateChanged)
    Q_PROPERTY(double  verticalSpeed   READ verticalSpeed   NOTIFY droneStateChanged)
    Q_PROPERTY(double  pitch           READ pitch           NOTIFY droneStateChanged)
    Q_PROPERTY(double  roll            READ roll            NOTIFY droneStateChanged)
    Q_PROPERTY(double  yaw             READ yaw             NOTIFY droneStateChanged)
    Q_PROPERTY(int     gpsSats         READ gpsSats         NOTIFY droneStateChanged)
    Q_PROPERTY(QString gpsFixType      READ gpsFixType      NOTIFY droneStateChanged)
    Q_PROPERTY(double  batteryVoltage  READ batteryVoltage  NOTIFY droneStateChanged)
    Q_PROPERTY(int     batteryPercent  READ batteryPercent  NOTIFY droneStateChanged)
    Q_PROPERTY(double  extVoltage      READ extVoltage      NOTIFY droneStateChanged)
    Q_PROPERTY(bool    mavlinkConnected READ mavlinkConnected NOTIFY linkStateChanged)
    Q_PROPERTY(bool    droneArmed      READ droneArmed      NOTIFY droneStateChanged)
    Q_PROPERTY(QString flightMode      READ flightMode      NOTIFY droneStateChanged)
    Q_PROPERTY(int     mavlinkRssi     READ mavlinkRssi     NOTIFY linkStateChanged)
    Q_PROPERTY(int     mavlinkSnr      READ mavlinkSnr      NOTIFY linkStateChanged)
    Q_PROPERTY(QStringList statusMessages READ statusMessages NOTIFY statusMessagesChanged)
    Q_PROPERTY(double  droneLat        READ droneLat        NOTIFY dronePositionChanged)
    Q_PROPERTY(double  droneLon        READ droneLon        NOTIFY dronePositionChanged)
    Q_PROPERTY(double  hdop            READ hdop            NOTIFY droneStateChanged)

    // --- Switch States (from Pico) ---
    Q_PROPERTY(bool    sw1Arm          READ sw1Arm          NOTIFY switchStateChanged)
    Q_PROPERTY(bool    sw1Rc           READ sw1Rc           NOTIFY switchStateChanged)
    Q_PROPERTY(bool    sw1Rth          READ sw1Rth          NOTIFY switchStateChanged)
    Q_PROPERTY(bool    sw3Pay1Arm      READ sw3Pay1Arm      NOTIFY switchStateChanged)
    Q_PROPERTY(bool    sw3Pay2Arm      READ sw3Pay2Arm      NOTIFY switchStateChanged)
    Q_PROPERTY(bool    sw3Pay1Fire     READ sw3Pay1Fire     NOTIFY switchStateChanged)
    Q_PROPERTY(bool    sw3Pay2Fire     READ sw3Pay2Fire     NOTIFY switchStateChanged)
    Q_PROPERTY(bool    crosshairActive READ crosshairActive WRITE setCrosshairActive NOTIFY switchStateChanged)
    Q_PROPERTY(QString previousFlightMode READ previousFlightMode NOTIFY switchStateChanged)

    // --- GCS Hardware (from Pico) ---
    Q_PROPERTY(bool    picoConnected   READ picoConnected   NOTIFY linkStateChanged)
    Q_PROPERTY(double  picoHeartbeatMs READ picoHeartbeatMs NOTIFY linkStateChanged)
    Q_PROPERTY(int     picoHeartbeatSeq READ picoHeartbeatSeq NOTIFY linkStateChanged)
    Q_PROPERTY(bool    keyUnlocked     READ keyUnlocked     NOTIFY keyStateChanged)
    Q_PROPERTY(double  alsLux          READ alsLux          NOTIFY sensorChanged)
    Q_PROPERTY(int     alsGain         READ alsGain         NOTIFY sensorChanged)
    Q_PROPERTY(int     alsIntMs        READ alsIntMs        NOTIFY sensorChanged)
    Q_PROPERTY(double  tempCaseA       READ tempCaseA       NOTIFY sensorChanged)
    Q_PROPERTY(double  tempCaseB       READ tempCaseB       NOTIFY sensorChanged)
    Q_PROPERTY(double  tempCaseC       READ tempCaseC       NOTIFY sensorChanged)
    Q_PROPERTY(double  tempCaseD       READ tempCaseD       NOTIFY sensorChanged)
    Q_PROPERTY(double  tempCpuPi       READ tempCpuPi       NOTIFY sensorChanged)

    // --- Warnings ---
    Q_PROPERTY(int warnTemp    READ warnTemp    NOTIFY warningsChanged)
    Q_PROPERTY(int warnSignal  READ warnSignal  NOTIFY warningsChanged)
    Q_PROPERTY(int warnDrone   READ warnDrone   NOTIFY warningsChanged)
    Q_PROPERTY(int warnGps     READ warnGps     NOTIFY warningsChanged)
    Q_PROPERTY(int warnLink    READ warnLink    NOTIFY warningsChanged)
    Q_PROPERTY(int warnNetwork READ warnNetwork NOTIFY warningsChanged)
    Q_PROPERTY(bool anyWarningActive READ anyWarningActive NOTIFY warningsChanged)

    // --- Brightness ---
    Q_PROPERTY(int  brightnessScreenL READ brightnessScreenL WRITE setBrightnessScreenL NOTIFY brightnessChanged)
    Q_PROPERTY(int  brightnessScreenR READ brightnessScreenR WRITE setBrightnessScreenR NOTIFY brightnessChanged)
    Q_PROPERTY(int  brightnessLed     READ brightnessLed     WRITE setBrightnessLed     NOTIFY brightnessChanged)
    Q_PROPERTY(int  brightnessTft     READ brightnessTft     WRITE setBrightnessTft     NOTIFY brightnessChanged)
    Q_PROPERTY(int  brightnessBtnLeds READ brightnessBtnLeds WRITE setBrightnessBtnLeds NOTIFY brightnessChanged)
    Q_PROPERTY(bool alsAutoEnabled    READ alsAutoEnabled    WRITE setAlsAutoEnabled    NOTIFY brightnessChanged)

    // --- Worklight ---
    Q_PROPERTY(bool   worklightOn    READ worklightOn    WRITE setWorklightOn    NOTIFY worklightChanged)
    Q_PROPERTY(QColor worklightColor READ worklightColor WRITE setWorklightColor NOTIFY worklightChanged)

    // --- Peripherals (RS-485) ---
    Q_PROPERTY(QVariantList peripherals READ peripherals NOTIFY peripheralsChanged)

    // --- System ---
    Q_PROPERTY(double uptimeSeconds  READ uptimeSeconds  NOTIFY systemChanged)
    Q_PROPERTY(int    memPercent     READ memPercent     NOTIFY systemChanged)
    Q_PROPERTY(int    diskPercent    READ diskPercent    NOTIFY systemChanged)

    // --- Mission ---
    Q_PROPERTY(QVariantList waypoints READ waypoints WRITE setWaypoints NOTIFY waypointsChanged)
    Q_PROPERTY(QVariantList pois      READ pois      WRITE setPois      NOTIFY poisChanged)

    // --- Parameters ---
    Q_PROPERTY(QVariantList params READ params NOTIFY paramsChanged)

public:
    static GCSState *instance();

    double  altitude()         const { return m_altitude; }
    double  groundSpeed()      const { return m_groundSpeed; }
    double  heading()          const { return m_heading; }
    double  verticalSpeed()    const { return m_verticalSpeed; }
    double  pitch()            const { return m_pitch; }
    double  roll()             const { return m_roll; }
    double  yaw()              const { return m_yaw; }
    int     gpsSats()          const { return m_gpsSats; }
    QString gpsFixType()       const { return m_gpsFixType; }
    double  batteryVoltage()   const { return m_batteryVoltage; }
    int     batteryPercent()   const { return m_batteryPercent; }
    double  extVoltage()       const { return m_extVoltage; }
    bool    mavlinkConnected() const { return m_mavlinkConnected; }
    bool    droneArmed()       const { return m_droneArmed; }
    QString flightMode()       const { return m_flightMode; }
    int     mavlinkRssi()      const { return m_mavlinkRssi; }
    int     mavlinkSnr()       const { return m_mavlinkSnr; }
    QStringList statusMessages() const { return m_statusMessages; }
    double  droneLat()         const { return m_droneLat; }
    double  droneLon()         const { return m_droneLon; }
    double  hdop()             const { return m_hdop; }
    bool    sw1Arm()           const { return m_sw1Arm; }
    bool    sw1Rc()            const { return m_sw1Rc; }
    bool    sw1Rth()           const { return m_sw1Rth; }
    bool    sw3Pay1Arm()       const { return m_sw3Pay1Arm; }
    bool    sw3Pay2Arm()       const { return m_sw3Pay2Arm; }
    bool    sw3Pay1Fire()      const { return m_sw3Pay1Fire; }
    bool    sw3Pay2Fire()      const { return m_sw3Pay2Fire; }
    bool    crosshairActive()  const { return m_crosshairActive; }
    QString previousFlightMode() const { return m_previousFlightMode; }
    bool    picoConnected()    const { return m_picoConnected; }
    double  picoHeartbeatMs()  const { return m_picoHeartbeatMs; }
    int     picoHeartbeatSeq() const { return m_picoHeartbeatSeq; }
    bool    keyUnlocked()      const { return m_keyUnlocked; }
    double  alsLux()           const { return m_alsLux; }
    int     alsGain()          const { return m_alsGain; }
    int     alsIntMs()         const { return m_alsIntMs; }
    double  tempCaseA()        const { return m_tempCaseA; }
    double  tempCaseB()        const { return m_tempCaseB; }
    double  tempCaseC()        const { return m_tempCaseC; }
    double  tempCaseD()        const { return m_tempCaseD; }
    double  tempCpuPi()        const { return m_tempCpuPi; }
    int     warnTemp()         const { return m_warnTemp; }
    int     warnSignal()       const { return m_warnSignal; }
    int     warnDrone()        const { return m_warnDrone; }
    int     warnGps()          const { return m_warnGps; }
    int     warnLink()         const { return m_warnLink; }
    int     warnNetwork()      const { return m_warnNetwork; }
    bool    anyWarningActive() const {
        return m_warnTemp > 0 || m_warnSignal > 0 || m_warnDrone > 0
            || m_warnGps > 0 || m_warnLink > 0 || m_warnNetwork > 0;
    }
    int  brightnessScreenL()   const { return m_brightnessScreenL; }
    int  brightnessScreenR()   const { return m_brightnessScreenR; }
    int  brightnessLed()       const { return m_brightnessLed; }
    int  brightnessTft()       const { return m_brightnessTft; }
    int  brightnessBtnLeds()   const { return m_brightnessBtnLeds; }
    bool alsAutoEnabled()      const { return m_alsAutoEnabled; }
    bool   worklightOn()       const { return m_worklightOn; }
    QColor worklightColor()    const { return m_worklightColor; }
    QVariantList peripherals() const { return m_peripherals; }
    double uptimeSeconds()     const { return m_uptimeSeconds; }
    int    memPercent()        const { return m_memPercent; }
    int    diskPercent()       const { return m_diskPercent; }
    QVariantList waypoints()   const { return m_waypoints; }
    QVariantList pois()        const { return m_pois; }
    QVariantList params()      const { return m_params; }

public slots:
    void updateDroneState(double alt, double spd, double hdg, double vspd,
                          double pitch, double roll, double yaw);
    void updateGps(int sats, const QString &fixType);
    void updateBattery(double voltage, int percent, double extV);
    void updateMavlinkLink(bool connected, int rssiDbm, int snrDb);
    void updateFlightMode(const QString &mode, bool armed);
    void appendStatusMessage(const QString &msg);
    void updatePicoLink(bool connected, double heartbeatMs, int seq);
    void updateKeyState(bool unlocked);
    void updateSwitchStates(int portA, int portB);
    Q_INVOKABLE void setCrosshairActive(bool active);
    void storePreviousFlightMode(const QString &mode);
    void updateAlsSensor(double lux, int gain, int intMs);
    void updateCaseTemps(double a, double b, double c, double d);
    void updateCpuTemp(double celsius);
    void updateWarnings(int temp, int signal, int drone, int gps, int link, int network);
    void updatePeripheral(int address, const QString &name, bool online,
                          const QVariantMap &deviceData);
    void updateSystemStats(double uptimeSec, int memPct, int diskPct);
    void setWaypoints(const QVariantList &wps);
    void setPois(const QVariantList &pois);
    void updateParams(const QVariantList &params);
    void setParamDirty(const QString &name, double newValue);
    void updateDronePosition(double lat, double lon);
    void updateHdop(double hdop);

    void setBrightnessScreenL(int v);
    void setBrightnessScreenR(int v);
    void setBrightnessLed(int v);
    void setBrightnessTft(int v);
    void setBrightnessBtnLeds(int v);
    void setAlsAutoEnabled(bool enabled);
    Q_INVOKABLE void setWorklightOn(bool on);
    Q_INVOKABLE void setWorklightColor(const QColor &color);

    Q_INVOKABLE void sendFlightMode(const QString &mode);
    Q_INVOKABLE void sendArmDisarm(bool arm);
    Q_INVOKABLE void sendMissionUpload();
    Q_INVOKABLE void sendMissionDownload();
    Q_INVOKABLE void sendParamWrite(const QString &name, double value);
    Q_INVOKABLE void sendParamRefresh();
    Q_INVOKABLE void sendPeriphCmd(int address, int cmd, const QByteArray &payload = {});
    Q_INVOKABLE void sendTftScreen(int mode);
    Q_INVOKABLE void sendTftPeriphDetail(int address);

signals:
    void droneStateChanged();
    void dronePositionChanged();
    void linkStateChanged();
    void keyStateChanged();
    void sensorChanged();
    void warningsChanged();
    void brightnessChanged();
    void worklightChanged();
    void peripheralsChanged();
    void systemChanged();
    void waypointsChanged();
    void poisChanged();
    void paramsChanged();
    void statusMessagesChanged();
    void switchStateChanged();
    void armSwitchToggled(bool on);
    void buttonPressed(int buttonId);
    void buttonReleased(int buttonId);
    // Internal command signals
    void cmdSetMode(const QString &mode);
    void cmdArmDisarm(bool arm);
    void cmdMissionUpload(const QVariantList &wps);
    void cmdMissionDownload();
    void cmdParamWrite(const QString &name, double value);
    void cmdParamRefresh();
    void cmdBrightnessChanged(int screenL, int screenR, int led, int tft, int btnLeds);
    void cmdWorklightChanged(bool on, const QColor &color);
    void cmdPeriphCmd(int address, int cmd, const QByteArray &payload);
    void cmdTftScreen(int mode);
    void cmdTftPeriphDetail(int address);

private:
    explicit GCSState(QObject *parent = nullptr);
    static GCSState *s_instance;

    double  m_altitude         = 0.0;
    double  m_groundSpeed      = 0.0;
    double  m_heading          = 0.0;
    double  m_verticalSpeed    = 0.0;
    double  m_pitch            = 0.0;
    double  m_roll             = 0.0;
    double  m_yaw              = 0.0;
    int     m_gpsSats          = 0;
    QString m_gpsFixType       = "NO-FIX";
    double  m_batteryVoltage   = 0.0;
    int     m_batteryPercent   = 0;
    double  m_extVoltage       = 0.0;
    bool    m_mavlinkConnected = false;
    bool    m_droneArmed       = false;
    QString m_flightMode       = "UNKNOWN";
    int     m_mavlinkRssi      = -127;
    int     m_mavlinkSnr       = 0;
    QStringList m_statusMessages;
    double  m_droneLat         = 0.0;
    double  m_droneLon         = 0.0;
    double  m_hdop             = 99.9;
    bool    m_sw1Arm           = false;
    bool    m_sw1Rc            = false;
    bool    m_sw1Rth           = false;
    bool    m_sw3Pay1Arm       = false;
    bool    m_sw3Pay2Arm       = false;
    bool    m_sw3Pay1Fire      = false;
    bool    m_sw3Pay2Fire      = false;
    bool    m_crosshairActive  = false;
    QString m_previousFlightMode;
    bool    m_picoConnected    = false;
    double  m_picoHeartbeatMs  = 0.0;
    int     m_picoHeartbeatSeq = 0;
    bool    m_keyUnlocked      = false;
    double  m_alsLux           = 0.0;
    int     m_alsGain          = 0;
    int     m_alsIntMs         = 100;
    double  m_tempCaseA        = qQNaN();
    double  m_tempCaseB        = qQNaN();
    double  m_tempCaseC        = qQNaN();
    double  m_tempCaseD        = qQNaN();
    double  m_tempCpuPi        = 0.0;
    int     m_warnTemp         = 0;
    int     m_warnSignal       = 0;
    int     m_warnDrone        = 0;
    int     m_warnGps          = 0;
    int     m_warnLink         = 0;
    int     m_warnNetwork      = 0;
    int     m_brightnessScreenL = 80;
    int     m_brightnessScreenR = 80;
    int     m_brightnessLed     = 50;
    int     m_brightnessTft     = 80;
    int     m_brightnessBtnLeds = 50;
    bool    m_alsAutoEnabled    = false;
    bool    m_worklightOn       = false;
    QColor  m_worklightColor    = QColor(255, 255, 255);
    QVariantList m_peripherals;
    double  m_uptimeSeconds    = 0.0;
    int     m_memPercent       = 0;
    int     m_diskPercent      = 0;
    QVariantList m_waypoints;
    QVariantList m_pois;
    QVariantList m_params;
};
