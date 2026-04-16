#include "gcsstate.h"
#include <QtMath>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

GCSState *GCSState::s_instance = nullptr;

GCSState *GCSState::instance()
{
    if (!s_instance)
        s_instance = new GCSState();
    return s_instance;
}

GCSState::GCSState(QObject *parent) : QObject(parent)
{
    m_caseTwinHotspots = defaultCaseTwinHotspots();
}

QVariantList GCSState::defaultCaseTwinHotspots()
{
    QVariantList out;
    out.append(QVariantMap{{"id", "pi_cpu"}, {"label", "PI CPU"}, {"sensorKey", "tempCpuPi"}, {"xNorm", 0.50}, {"yNorm", 0.38}, {"x3d", 0.000}, {"y3d", 0.170}, {"z3d", 0.226}, {"okMax", 60.0}, {"warnMax", 80.0}});
    out.append(QVariantMap{{"id", "pcb_power"}, {"label", "PCB POWER"}, {"sensorKey", "tempCaseA"}, {"xNorm", 0.25}, {"yNorm", 0.52}, {"x3d", -0.116}, {"y3d", 0.170}, {"z3d", 0.172}, {"okMax", 60.0}, {"warnMax", 80.0}});
    out.append(QVariantMap{{"id", "raspberry_pi_zone"}, {"label", "RASPBERRY PI"}, {"sensorKey", "tempCaseB"}, {"xNorm", 0.50}, {"yNorm", 0.48}, {"x3d", 0.000}, {"y3d", 0.170}, {"z3d", 0.187}, {"okMax", 60.0}, {"warnMax", 80.0}});
    out.append(QVariantMap{{"id", "charger"}, {"label", "CHARGER"}, {"sensorKey", "tempCaseC"}, {"xNorm", 0.30}, {"yNorm", 0.62}, {"x3d", -0.093}, {"y3d", 0.170}, {"z3d", 0.133}, {"okMax", 60.0}, {"warnMax", 80.0}});
    out.append(QVariantMap{{"id", "vrx_module"}, {"label", "VRX MODULE"}, {"sensorKey", "tempCaseD"}, {"xNorm", 0.72}, {"yNorm", 0.55}, {"x3d", 0.102}, {"y3d", 0.170}, {"z3d", 0.160}, {"okMax", 60.0}, {"warnMax", 80.0}});
    out.append(QVariantMap{{"id", "battery_bus"}, {"label", "BATTERY BUS"}, {"sensorKey", "batteryVoltage"}, {"xNorm", 0.56}, {"yNorm", 0.66}, {"x3d", 0.028}, {"y3d", 0.170}, {"z3d", 0.117}, {"metricType", "voltage"}, {"unit", "V"}, {"okMax", 30.0}, {"warnMax", 32.0}, {"warnMin", 22.0}, {"critMin", 20.0}, {"decimals", 1}});
    out.append(QVariantMap{{"id", "mavlink_link"}, {"label", "MAVLINK"}, {"sensorKey", "mavlinkConnected"}, {"xNorm", 0.68}, {"yNorm", 0.42}, {"x3d", 0.084}, {"y3d", 0.170}, {"z3d", 0.211}, {"metricType", "bool"}, {"trueLabel", "ONLINE"}, {"falseLabel", "OFFLINE"}, {"invert", false}});
    return out;
}

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

bool GCSState::loadCaseTwinConfig(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_caseTwinConfigLastError = QString("Cannot open file: %1").arg(path);
        emit caseTwinConfigChanged();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        m_caseTwinConfigLastError = QString("Invalid JSON: %1").arg(parseError.errorString());
        emit caseTwinConfigChanged();
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonArray hotspots = root.value("hotspots").toArray();
    if (hotspots.isEmpty()) {
        m_caseTwinConfigLastError = "Config missing non-empty 'hotspots' array";
        emit caseTwinConfigChanged();
        return false;
    }

    QVariantList parsed;
    for (int i = 0; i < hotspots.size(); ++i) {
        const QJsonObject h = hotspots[i].toObject();
        const QString id = h.value("id").toString().trimmed();
        const QString label = h.value("label").toString().trimmed();
        const QString sensorKey = h.value("sensor_key").toString(h.value("sensorKey").toString()).trimmed();
        const double xNorm = h.value("x_norm").toDouble(h.value("xNorm").toDouble(-1.0));
        const double yNorm = h.value("y_norm").toDouble(h.value("yNorm").toDouble(-1.0));
        const double okMax = h.value("ok_max").toDouble(h.value("okMax").toDouble(60.0));
        const double warnMax = h.value("warn_max").toDouble(h.value("warnMax").toDouble(80.0));
        const double warnMin = h.value("warn_min").toDouble(h.value("warnMin").toDouble(qQNaN()));
        const double critMin = h.value("crit_min").toDouble(h.value("critMin").toDouble(qQNaN()));
        const int decimals = h.value("decimals").toInt(-9999);
        QString metricType = h.value("metric_type").toString(h.value("metricType").toString("temp")).trimmed();
        if (metricType.isEmpty()) metricType = "temp";
        const QString unit = h.value("unit").toString();
        const QString trueLabel = h.value("true_label").toString(h.value("trueLabel").toString());
        const QString falseLabel = h.value("false_label").toString(h.value("falseLabel").toString());
        const bool invert = h.value("invert").toBool(false);
        const double x3d = h.value("x3d").toDouble(qQNaN());
        const double y3d = h.value("y3d").toDouble(qQNaN());
        const double z3d = h.value("z3d").toDouble(qQNaN());

        if (id.isEmpty() || label.isEmpty() || sensorKey.isEmpty() || xNorm < 0.0 || xNorm > 1.0 || yNorm < 0.0 || yNorm > 1.0) {
            m_caseTwinConfigLastError = QString("Invalid hotspot at index %1").arg(i);
            emit caseTwinConfigChanged();
            return false;
        }

        parsed.append(QVariantMap{
            {"id", id},
            {"label", label},
            {"sensorKey", sensorKey},
            {"xNorm", xNorm},
            {"yNorm", yNorm},
            {"metricType", metricType},
            {"unit", unit},
            {"okMax", okMax},
            {"warnMax", warnMax},
            {"warnMin", warnMin},
            {"critMin", critMin},
            {"decimals", decimals},
            {"trueLabel", trueLabel},
            {"falseLabel", falseLabel},
            {"invert", invert},
            {"x3d", x3d},
            {"y3d", y3d},
            {"z3d", z3d}
        });
    }

    m_caseTwinHotspots = parsed;
    m_caseTwinConfigLastError.clear();
    emit caseTwinConfigChanged();
    return true;
}

bool GCSState::saveCaseTwinConfig(const QString &path)
{
    QJsonArray hotspots;
    for (const QVariant &entryVar : m_caseTwinHotspots) {
        const QVariantMap entry = entryVar.toMap();
        QJsonObject h;
        h["id"] = entry.value("id").toString();
        h["label"] = entry.value("label").toString();
        h["sensor_key"] = entry.value("sensorKey").toString();
        h["x_norm"] = entry.value("xNorm").toDouble();
        h["y_norm"] = entry.value("yNorm").toDouble();
        const QString metricType = entry.value("metricType").toString().isEmpty() ? "temp" : entry.value("metricType").toString();
        h["metric_type"] = metricType;
        h["unit"] = entry.value("unit").toString();
        h["ok_max"] = entry.value("okMax").toDouble();
        h["warn_max"] = entry.value("warnMax").toDouble();
        if (!qIsNaN(entry.value("warnMin").toDouble())) h["warn_min"] = entry.value("warnMin").toDouble();
        if (!qIsNaN(entry.value("critMin").toDouble())) h["crit_min"] = entry.value("critMin").toDouble();
        { bool ok = false; int dec = entry.value("decimals").toInt(&ok); if (ok) h["decimals"] = dec; }
        if (!entry.value("trueLabel").toString().isEmpty()) h["true_label"] = entry.value("trueLabel").toString();
        if (!entry.value("falseLabel").toString().isEmpty()) h["false_label"] = entry.value("falseLabel").toString();
        if (entry.contains("invert")) h["invert"] = entry.value("invert").toBool();
        if (!qIsNaN(entry.value("x3d").toDouble())) h["x3d"] = entry.value("x3d").toDouble();
        if (!qIsNaN(entry.value("y3d").toDouble())) h["y3d"] = entry.value("y3d").toDouble();
        if (!qIsNaN(entry.value("z3d").toDouble())) h["z3d"] = entry.value("z3d").toDouble();
        hotspots.append(h);
    }

    QJsonObject root;
    root["version"] = 1;
    root["hotspots"] = hotspots;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        m_caseTwinConfigLastError = QString("Cannot write file: %1").arg(path);
        emit caseTwinConfigChanged();
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    m_caseTwinConfigLastError.clear();
    emit caseTwinConfigChanged();
    return true;
}

void GCSState::resetCaseTwinConfig()
{
    m_caseTwinHotspots = defaultCaseTwinHotspots();
    m_caseTwinConfigLastError.clear();
    emit caseTwinConfigChanged();
}
