#include "gcsstate.h"
#include <QtMath>

GCSState *GCSState::s_instance = nullptr;

GCSState *GCSState::instance()
{
    if (!s_instance)
        s_instance = new GCSState();
    return s_instance;
}

GCSState::GCSState(QObject *parent) : QObject(parent) {}

void GCSState::updateDroneState(double alt, double spd, double hdg, double vspd,
                                 double pitch, double roll, double yaw)
{
    m_altitude      = alt;
    m_groundSpeed   = spd;
    m_heading       = hdg;
    m_verticalSpeed = vspd;
    m_pitch         = pitch;
    m_roll          = roll;
    m_yaw           = yaw;
    emit droneStateChanged();
}

void GCSState::updateGps(int sats, const QString &fixType)
{
    m_gpsSats    = sats;
    m_gpsFixType = fixType;
    emit droneStateChanged();
}

void GCSState::updateBattery(double voltage, int percent, double extV)
{
    m_batteryVoltage = voltage;
    m_batteryPercent = percent;
    m_extVoltage     = extV;
    emit droneStateChanged();
}

void GCSState::updateMavlinkLink(bool connected, int rssiDbm, int snrDb)
{
    m_mavlinkConnected = connected;
    m_mavlinkRssi      = rssiDbm;
    m_mavlinkSnr       = snrDb;
    emit linkStateChanged();
}

void GCSState::updateFlightMode(const QString &mode, bool armed)
{
    m_flightMode = mode;
    m_droneArmed = armed;
    emit droneStateChanged();
}

void GCSState::appendStatusMessage(const QString &msg)
{
    m_statusMessages.prepend(msg);
    while (m_statusMessages.size() > 3)
        m_statusMessages.removeLast();
    emit statusMessagesChanged();
}

void GCSState::updatePicoLink(bool connected, double heartbeatMs, int seq)
{
    m_picoConnected    = connected;
    m_picoHeartbeatMs  = heartbeatMs;
    m_picoHeartbeatSeq = seq;
    emit linkStateChanged();
}

void GCSState::updateKeyState(bool unlocked)
{
    if (m_keyUnlocked == unlocked) return;
    m_keyUnlocked = unlocked;
    emit keyStateChanged();
}

void GCSState::updateSwitchStates(int portA, int portB)
{
    // Active-low: bit=0 means ON
    bool newSw1Arm     = !(portB & (1 << 0));
    bool newSw1Rc      = !(portB & (1 << 1));
    bool newSw1Rth     = !(portB & (1 << 2));
    bool newPay1Arm    = !(portB & (1 << 6));
    bool newPay2Arm    = !(portB & (1 << 7));
    bool newPay1Fire   = !(portA & (1 << 6));
    bool newPay2Fire   = !(portA & (1 << 7));

    bool armEdge = (newSw1Arm != m_sw1Arm);

    m_sw1Arm      = newSw1Arm;
    m_sw1Rc       = newSw1Rc;
    m_sw1Rth      = newSw1Rth;
    m_sw3Pay1Arm  = newPay1Arm;
    m_sw3Pay2Arm  = newPay2Arm;
    m_sw3Pay1Fire = newPay1Fire;
    m_sw3Pay2Fire = newPay2Fire;

    emit switchStateChanged();

    if (armEdge)
        emit armSwitchToggled(m_sw1Arm);
}

void GCSState::setCrosshairActive(bool active)
{
    if (m_crosshairActive == active) return;
    m_crosshairActive = active;
    emit switchStateChanged();
}

void GCSState::storePreviousFlightMode(const QString &mode)
{
    m_previousFlightMode = mode;
}

void GCSState::updateAlsSensor(double lux, int gain, int intMs)
{
    m_alsLux   = lux;
    m_alsGain  = gain;
    m_alsIntMs = intMs;
    emit sensorChanged();
}

void GCSState::updateCaseTemps(double a, double b, double c, double d)
{
    m_tempCaseA = a;
    m_tempCaseB = b;
    m_tempCaseC = c;
    m_tempCaseD = d;
    emit sensorChanged();
}

void GCSState::updateCpuTemp(double celsius)
{
    m_tempCpuPi = celsius;
    emit sensorChanged();
}

void GCSState::updateWarnings(int temp, int signal, int drone, int gps, int link, int network)
{
    m_warnTemp    = temp;
    m_warnSignal  = signal;
    m_warnDrone   = drone;
    m_warnGps     = gps;
    m_warnLink    = link;
    m_warnNetwork = network;
    emit warningsChanged();
}

void GCSState::updatePeripheral(int address, const QString &name, bool online,
                                 const QVariantMap &deviceData)
{
    for (int i = 0; i < m_peripherals.size(); ++i) {
        QVariantMap entry = m_peripherals[i].toMap();
        if (entry["address"].toInt() == address) {
            entry["name"]   = name;
            entry["online"] = online;
            for (auto it = deviceData.begin(); it != deviceData.end(); ++it)
                entry[it.key()] = it.value();
            m_peripherals[i] = entry;
            emit peripheralsChanged();
            return;
        }
    }
    QVariantMap entry;
    entry["address"]   = address;
    entry["name"]      = name;
    entry["online"]    = online;
    entry["tftActive"] = false;
    for (auto it = deviceData.begin(); it != deviceData.end(); ++it)
        entry[it.key()] = it.value();
    m_peripherals.append(entry);
    emit peripheralsChanged();
}

void GCSState::updateSystemStats(double uptimeSec, int memPct, int diskPct)
{
    m_uptimeSeconds = uptimeSec;
    m_memPercent    = memPct;
    m_diskPercent   = diskPct;
    emit systemChanged();
}

void GCSState::setWaypoints(const QVariantList &wps)
{
    m_waypoints = wps;
    emit waypointsChanged();
}

void GCSState::setPois(const QVariantList &pois)
{
    m_pois = pois;
    emit poisChanged();
}

void GCSState::updateParams(const QVariantList &params)
{
    m_params = params;
    emit paramsChanged();
}

void GCSState::setParamDirty(const QString &name, double newValue)
{
    for (int i = 0; i < m_params.size(); ++i) {
        QVariantMap p = m_params[i].toMap();
        if (p["name"].toString() == name) {
            p["value"] = newValue;
            p["dirty"] = true;
            m_params[i] = p;
            emit paramsChanged();
            return;
        }
    }
}

void GCSState::updateDronePosition(double lat, double lon)
{
    m_droneLat = lat;
    m_droneLon = lon;
    emit dronePositionChanged();
}

void GCSState::updateHdop(double hdop)
{
    m_hdop = hdop;
    emit droneStateChanged();
}

void GCSState::setBrightnessScreenL(int v)
{
    m_brightnessScreenL = qBound(0, v, 100);
    emit brightnessChanged();
    emit cmdBrightnessChanged(m_brightnessScreenL, m_brightnessScreenR,
                              m_brightnessLed, m_brightnessTft, m_brightnessBtnLeds);
}

void GCSState::setBrightnessScreenR(int v)
{
    m_brightnessScreenR = qBound(0, v, 100);
    emit brightnessChanged();
    emit cmdBrightnessChanged(m_brightnessScreenL, m_brightnessScreenR,
                              m_brightnessLed, m_brightnessTft, m_brightnessBtnLeds);
}

void GCSState::setBrightnessLed(int v)
{
    m_brightnessLed = qBound(0, v, 100);
    emit brightnessChanged();
    emit cmdBrightnessChanged(m_brightnessScreenL, m_brightnessScreenR,
                              m_brightnessLed, m_brightnessTft, m_brightnessBtnLeds);
}

void GCSState::setBrightnessTft(int v)
{
    m_brightnessTft = qBound(0, v, 100);
    emit brightnessChanged();
    emit cmdBrightnessChanged(m_brightnessScreenL, m_brightnessScreenR,
                              m_brightnessLed, m_brightnessTft, m_brightnessBtnLeds);
}

void GCSState::setBrightnessBtnLeds(int v)
{
    m_brightnessBtnLeds = qBound(0, v, 100);
    emit brightnessChanged();
    emit cmdBrightnessChanged(m_brightnessScreenL, m_brightnessScreenR,
                              m_brightnessLed, m_brightnessTft, m_brightnessBtnLeds);
}

void GCSState::setAlsAutoEnabled(bool enabled)
{
    m_alsAutoEnabled = enabled;
    emit brightnessChanged();
}

void GCSState::setWorklightOn(bool on)
{
    if (m_worklightOn == on) return;
    m_worklightOn = on;
    emit worklightChanged();
    emit cmdWorklightChanged(m_worklightOn, m_worklightColor);
}

void GCSState::setWorklightColor(const QColor &color)
{
    if (m_worklightColor == color) return;
    m_worklightColor = color;
    emit worklightChanged();
    if (m_worklightOn)
        emit cmdWorklightChanged(m_worklightOn, m_worklightColor);
}

void GCSState::sendFlightMode(const QString &mode)
{
    emit cmdSetMode(mode);
}

void GCSState::sendArmDisarm(bool arm)
{
    emit cmdArmDisarm(arm);
}

void GCSState::sendMissionUpload()
{
    emit cmdMissionUpload(m_waypoints);
}

void GCSState::sendMissionDownload()
{
    emit cmdMissionDownload();
}

void GCSState::sendParamWrite(const QString &name, double value)
{
    emit cmdParamWrite(name, value);
}

void GCSState::sendParamRefresh()
{
    emit cmdParamRefresh();
}

void GCSState::sendPeriphCmd(int address, int cmd, const QByteArray &payload)
{
    emit cmdPeriphCmd(address, cmd, payload);
}

void GCSState::sendTftScreen(int mode)
{
    emit cmdTftScreen(mode);
}

void GCSState::sendTftPeriphDetail(int address)
{
    emit cmdTftPeriphDetail(address);
}
