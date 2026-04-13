#include "picolink.h"
#include <QDateTime>
#include <QtMath>
#include <cstring>
#include <QFile>
#include <QStringList>

#define PROTO_SOF               0xAA
#define PROTO_TYPE_ADC          0x01
#define PROTO_TYPE_DIGITAL      0x02
#define PROTO_TYPE_BRIGHTNESS   0x08
#define PROTO_TYPE_WARNING      0x0A
#define PROTO_TYPE_ALS          0x0B
#define PROTO_TYPE_HEARTBEAT    0x05
#define PROTO_TYPE_EVENT        0x06
#define PROTO_TYPE_PERIPH_DATA  0x0D
#define PROTO_TYPE_PERIPH_STATE 0x0E
#define PROTO_TYPE_PERIPH_CMD   0x0C
#define PROTO_TYPE_LED            0x03
#define PROTO_TYPE_SCREEN         0x04
#define PROTO_TYPE_PERIPH_SCREEN  0x0F

#define ADC_CH_BAT_VIN   0
#define ADC_CH_EXT_VIN   1
#define ADC_CH_SENS2     2
#define ADC_CH_SENS3     3
#define ADC_CH_SENS4     4
#define ADC_CH_SENS5     5

PicoLink::PicoLink(GCSState *state, QObject *parent)
    : QObject(parent), m_state(state)
{
    m_serial.setPortName("/dev/ttyACM0");
    m_serial.setBaudRate(QSerialPort::Baud115200);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setParity(QSerialPort::NoParity);

    connect(&m_serial, &QSerialPort::readyRead, this, &PicoLink::onReadyRead);
    connect(&m_serial, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, &PicoLink::onSerialError);

    m_heartbeatTimer.setInterval(1000);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &PicoLink::sendHeartbeat);

    m_cpuTempTimer.setInterval(2000);
    connect(&m_cpuTempTimer, &QTimer::timeout, this, &PicoLink::readCpuTemp);

    m_statsTimer.setInterval(5000);
    connect(&m_statsTimer, &QTimer::timeout, this, &PicoLink::readSystemStats);

    m_retryTimer.setInterval(2000);
    connect(&m_retryTimer, &QTimer::timeout, this, &PicoLink::retryConnect);

    connect(m_state, &GCSState::cmdBrightnessChanged, this, &PicoLink::onBrightnessChanged);
    connect(m_state, &GCSState::cmdWorklightChanged, this, &PicoLink::onWorklightChanged);
    connect(m_state, &GCSState::cmdPeriphCmd,       this, &PicoLink::onPeriphCmd);
    connect(m_state, &GCSState::cmdTftScreen,       this, &PicoLink::onTftScreen);
    connect(m_state, &GCSState::cmdTftPeriphDetail, this, &PicoLink::onTftPeriphDetail);
}

void PicoLink::start()
{
    openPort();
    m_heartbeatTimer.start();
    m_cpuTempTimer.start();
    m_statsTimer.start();
}

void PicoLink::openPort()
{
    if (m_serial.open(QIODevice::ReadWrite)) {
        m_connected = true;
        m_lastHeartbeatRecv = QDateTime::currentDateTime();
        m_retryTimer.stop();
    } else {
        m_connected = false;
        m_state->updatePicoLink(false, 0, 0);
        m_retryTimer.start();
    }
}

void PicoLink::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        if (m_serial.isOpen())
            m_serial.close();
        m_connected = false;
        m_state->updatePicoLink(false, 0, 0);
        m_retryTimer.start();
    }
}

void PicoLink::retryConnect()
{
    if (!m_connected)
        openPort();
}

void PicoLink::sendHeartbeat()
{
    if (!m_connected) return;

    m_lastHeartbeatSent = QDateTime::currentDateTime();

    if (m_lastHeartbeatRecv.isValid() &&
        m_lastHeartbeatSent.msecsTo(QDateTime::currentDateTime()) > 3000) {
        m_connected = false;
        m_state->updatePicoLink(false, 0, 0);
        return;
    }

    uint8_t payload[1];
    payload[0] = m_heartbeatSeqTx++;
    sendFrame(PROTO_TYPE_HEARTBEAT, payload, 1);
}

void PicoLink::readCpuTemp()
{
    QFile file("/sys/class/thermal/thermal_zone0/temp");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString line = file.readLine();
        file.close();
        bool ok;
        double tempMilli = line.toDouble(&ok);
        if (ok)
            m_state->updateCpuTemp(tempMilli / 1000.0);
    }
}

void PicoLink::readSystemStats()
{
    QFile uptimeFile("/proc/uptime");
    if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString line = uptimeFile.readLine();
        uptimeFile.close();
        QStringList parts = line.split(' ');
        if (parts.size() > 0) {
            bool ok;
            double uptime = parts[0].toDouble(&ok);
            if (ok)
                m_state->updateSystemStats(uptime, 0, 0);
        }
    }

    QFile meminfoFile("/proc/meminfo");
    if (meminfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString memTotal, memAvail;
        while (!meminfoFile.atEnd()) {
            QString line = meminfoFile.readLine();
            if (line.startsWith("MemTotal:"))
                memTotal = line;
            else if (line.startsWith("MemAvailable:"))
                memAvail = line;
        }
        meminfoFile.close();

        bool ok1, ok2;
        int total = memTotal.split(':')[1].split(' ', Qt::SkipEmptyParts)[0].toInt(&ok1);
        int avail = memAvail.split(':')[1].split(' ', Qt::SkipEmptyParts)[0].toInt(&ok2);
        if (ok1 && ok2) {
            int memPct = 100 - (avail * 100 / total);
            m_state->updateSystemStats(m_state->uptimeSeconds(), qBound(0, memPct, 100), 0);
        }
    }
}

void PicoLink::onReadyRead()
{
    m_rxBuf.append(m_serial.readAll());

    while (m_rxBuf.size() >= 5) {
        if (m_rxBuf[0] != PROTO_SOF) {
            m_rxBuf.remove(0, 1);
            continue;
        }

        uint8_t type = m_rxBuf[1];
        uint8_t lenLo = m_rxBuf[2];
        uint8_t lenHi = m_rxBuf[3];
        uint16_t payloadLen = (uint16_t)lenLo | ((uint16_t)lenHi << 8);

        if (m_rxBuf.size() < (int)(5 + payloadLen))
            break;

        uint8_t cksum = type ^ lenLo ^ lenHi;
        for (uint16_t i = 0; i < payloadLen; i++)
            cksum ^= m_rxBuf[4 + i];

        uint8_t rxCksum = m_rxBuf[4 + payloadLen];
        if (cksum != rxCksum) {
            m_rxBuf.remove(0, 5 + payloadLen);
            continue;
        }

        processPacket(type, (const uint8_t *)m_rxBuf.constData() + 4, payloadLen);
        m_rxBuf.remove(0, 5 + payloadLen);
    }
}

void PicoLink::processPacket(uint8_t type, const uint8_t *payload, int len)
{
    switch (type) {
    case PROTO_TYPE_ADC:
        handleAdcPacket(payload, len);
        break;
    case PROTO_TYPE_DIGITAL:
        handleDigitalPacket(payload, len);
        break;
    case PROTO_TYPE_HEARTBEAT:
        handleHeartbeatPacket(payload, len);
        break;
    case PROTO_TYPE_ALS:
        handleAlsPacket(payload, len);
        break;
    case PROTO_TYPE_EVENT:
        handleEventPacket(payload, len);
        break;
    case PROTO_TYPE_PERIPH_DATA:
        handlePeriphDataPacket(payload, len);
        break;
    case PROTO_TYPE_PERIPH_STATE:
        handlePeriphStatePacket(payload, len);
        break;
    }
}

void PicoLink::handleAdcPacket(const uint8_t *payload, int len)
{
    if (len < 14) return;

    uint16_t ch[6];
    memcpy(ch, payload, 12);

    double batV = adcToVolts(ch[ADC_CH_BAT_VIN], BAT_DIVIDER);
    double extV = adcToVolts(ch[ADC_CH_EXT_VIN], EXT_DIVIDER);
    int batPct = batteryPercent(batV);

    m_state->updateBattery(batV, batPct, extV);

    double tempA = ntcTocelsius(ch[ADC_CH_SENS2]);
    double tempB = ntcTocelsius(ch[ADC_CH_SENS3]);
    double tempC = ntcTocelsius(ch[ADC_CH_SENS4]);
    double tempD = ntcTocelsius(ch[ADC_CH_SENS5]);

    m_state->updateCaseTemps(tempA, tempB, tempC, tempD);
}

void PicoLink::handleDigitalPacket(const uint8_t *payload, int len)
{
    if (len < 4) return;
    uint8_t portA = payload[0];
    uint8_t portB = payload[1];

    // Key state: active-low, bit 5 of port A
    m_state->updateKeyState(!(portA & (1 << 5)));

    // Update all switch states
    m_state->updateSwitchStates(portA, portB);

    if (!m_initialSyncDone) {
        m_lastPortA = portA;
        m_lastPortB = portB;
        m_initialSyncDone = true;
        updatePayloadLeds();
        return;
    }

    handleSwitchLogic(portA, portB);
    m_lastPortA = portA;
    m_lastPortB = portB;
}

void PicoLink::handleHeartbeatPacket(const uint8_t *payload, int len)
{
    if (len < 1) return;
    m_lastHeartbeatRecv = QDateTime::currentDateTime();
    m_connected = true;
    double elapsed = m_lastHeartbeatSent.msecsTo(m_lastHeartbeatRecv);
    m_state->updatePicoLink(true, qAbs(elapsed), payload[0]);
    updateTftMode();
}

void PicoLink::handleAlsPacket(const uint8_t *payload, int len)
{
    if (len < 8) return;
    uint32_t luxMilli;
    memcpy(&luxMilli, payload + 4, 4);
    double lux = luxMilli / 1000.0;
    m_state->updateAlsSensor(lux, 0, 100);
}

void PicoLink::handleEventPacket(const uint8_t *payload, int len)
{
    if (len < 3) return;
    uint8_t eventId = payload[0];
    uint16_t value;
    memcpy(&value, payload + 1, 2);

    switch (eventId) {
    case 0x01: { // EVT_SWITCH_CHANGED
        uint8_t portA = (uint8_t)(value >> 8);
        uint8_t portB = (uint8_t)(value & 0xFF);
        m_state->updateKeyState(!(portA & (1 << 5)));
        m_state->updateSwitchStates(portA, portB);
        if (m_initialSyncDone)
            handleSwitchLogic(portA, portB);
        m_lastPortA = portA;
        m_lastPortB = portB;
        break;
    }
    case 0x02: // EVT_BUTTON_PRESSED
        emit m_state->buttonPressed(value);
        handleButtonPress(value);
        break;
    case 0x03: // EVT_BUTTON_RELEASED
        emit m_state->buttonReleased(value);
        handleButtonRelease(value);
        break;
    case 0x04: // EVT_KEY_LOCK_CHANGED
        m_state->updateKeyState(value != 0);
        break;
    }
}

void PicoLink::handlePeriphDataPacket(const uint8_t *payload, int len)
{
    if (len < 3) return;
    uint8_t addr = payload[0];
    uint8_t cmd = payload[1];
    uint8_t dataLen = payload[2];

    const uint8_t *data = (dataLen > 0) ? payload + 3 : nullptr;
    if (dataLen > 0 && len < (3 + dataLen))
        dataLen = len - 3;

    if (addr == 0x01 && dataLen >= 3) {
        QVariantMap deviceData;
        deviceData["brightness"] = data[0];
        deviceData["temp"] = static_cast<int8_t>(data[1]);
        deviceData["faults"] = data[2];
        m_state->updatePeripheral(addr, "SEARCHLIGHT", true, deviceData);
    }
}

void PicoLink::handlePeriphStatePacket(const uint8_t *payload, int len)
{
    if (len < 2) return;
    uint8_t addr = payload[0];
    uint8_t online = payload[1];

    QVariantMap empty;
    m_state->updatePeripheral(addr, QString("DEV_0x%1").arg(addr, 2, 16, QChar('0')),
                              online != 0, empty);
}

double PicoLink::ntcTocelsius(uint16_t rawAdc) const
{
    if (rawAdc == 0) return qQNaN();

    double v = adcToVolts(rawAdc, 1.0);
    double r = (v * NTC_RFIXED) / (ADC_VREF - v);

    if (r <= 0) return qQNaN();

    double lnR = qLn(r / NTC_R0);
    double invT = (1.0 / NTC_T0) + (lnR / NTC_BETA);
    double celsius = (1.0 / invT) - 273.15;

    return celsius;
}

double PicoLink::adcToVolts(uint16_t raw, double divider) const
{
    return ((double)raw / ADC_MAX) * ADC_VREF * divider;
}

int PicoLink::batteryPercent(double voltage) const
{
    if (voltage >= BAT_FULL_V)
        return 100;
    if (voltage <= BAT_EMPTY_V)
        return 0;
    return static_cast<int>(((voltage - BAT_EMPTY_V) / (BAT_FULL_V - BAT_EMPTY_V)) * 100);
}

int PicoLink::buildFrame(uint8_t *buf, int bufSize, uint8_t type,
                         const uint8_t *payload, uint16_t payloadLen)
{
    int total = 5 + payloadLen;
    if (bufSize < total) return -1;

    uint8_t lenLo = (uint8_t)(payloadLen & 0xFF);
    uint8_t lenHi = (uint8_t)(payloadLen >> 8);

    buf[0] = PROTO_SOF;
    buf[1] = type;
    buf[2] = lenLo;
    buf[3] = lenHi;
    if (payloadLen > 0)
        memcpy(&buf[4], payload, payloadLen);

    uint8_t cksum = type ^ lenLo ^ lenHi;
    for (uint16_t i = 0; i < payloadLen; i++)
        cksum ^= payload[i];
    buf[4 + payloadLen] = cksum;

    return total;
}

void PicoLink::sendFrame(uint8_t type, const uint8_t *payload, uint16_t payloadLen)
{
    if (!m_connected) return;

    uint8_t buf[520];
    int frameLen = buildFrame(buf, sizeof(buf), type, payload, payloadLen);
    if (frameLen > 0)
        m_serial.write((const char *)buf, frameLen);
}

void PicoLink::onBrightnessChanged(int screenL, int screenR, int led, int tft, int btnLeds)
{
    if (!m_connected) return;

    uint8_t brightness[10];
    brightness[0] = 0;
    brightness[1] = screenL * 255 / 100;
    brightness[2] = 1;
    brightness[3] = screenR * 255 / 100;
    brightness[4] = 2;
    brightness[5] = tft * 255 / 100;

    sendFrame(PROTO_TYPE_BRIGHTNESS, brightness, 6);
}

void PicoLink::onPeriphCmd(int address, int cmd, const QByteArray &payload)
{
    if (!m_connected) return;

    uint8_t buf[260];
    buf[0] = address;
    buf[1] = cmd;
    buf[2] = payload.size();
    if (payload.size() > 0)
        memcpy(&buf[3], payload.constData(), payload.size());

    sendFrame(PROTO_TYPE_PERIPH_CMD, buf, 3 + payload.size());
}

void PicoLink::updateTftMode()
{
    int mode = 1;

    if (m_state->batteryPercent() < 15)
        mode = 4;
    else if (m_state->anyWarningActive())
        mode = 2;
    else if (!m_state->keyUnlocked())
        mode = 3;
    else if (m_state->peripherals().size() > 0)
        mode = 5;
    else
        mode = 1;

    if (mode != m_lastTftMode) {
        m_lastTftMode = mode;
        uint8_t modeVal = static_cast<uint8_t>(mode);
        sendFrame(PROTO_TYPE_SCREEN, &modeVal, 1);
    }
}

void PicoLink::onTftScreen(int mode)
{
    uint8_t modeVal = static_cast<uint8_t>(mode);
    sendFrame(PROTO_TYPE_SCREEN, &modeVal, 1);
    m_lastTftMode = mode;
}

void PicoLink::onTftPeriphDetail(int address)
{
    uint8_t addr = static_cast<uint8_t>(address);
    sendFrame(PROTO_TYPE_PERIPH_SCREEN, &addr, 1);
}

void PicoLink::handleSwitchLogic(uint8_t portA, uint8_t portB)
{
    uint8_t changedB = portB ^ m_lastPortB;
    uint8_t changedA = portA ^ m_lastPortA;

    // SW1_1 ARM — port B bit 0 (active-low: bit=0 means ON)
    if (changedB & (1 << 0)) {
        bool on = !(portB & (1 << 0));
        if (on) {
            // armSwitchToggled is emitted by GCSState::updateSwitchStates
            // QML handles the confirm overlay
        } else {
            if (m_state->keyUnlocked() && m_state->droneArmed())
                m_state->sendArmDisarm(false);
        }
    }

    // SW1_2 RC — port B bit 1
    if (changedB & (1 << 1)) {
        bool on = !(portB & (1 << 1));
        if (m_state->keyUnlocked()) {
            if (on) {
                bool rthActive = !(portB & (1 << 2));
                if (!rthActive)
                    m_previousFlightMode = m_state->flightMode();
                m_state->storePreviousFlightMode(m_previousFlightMode);
                m_state->sendFlightMode("GUIDED");
            } else {
                bool rthActive = !(portB & (1 << 2));
                if (rthActive)
                    m_state->sendFlightMode("RTL");
                else
                    m_state->sendFlightMode(m_previousFlightMode.isEmpty() ? "LOITER" : m_previousFlightMode);
            }
        }
    }

    // SW1_3 RTH — port B bit 2
    if (changedB & (1 << 2)) {
        bool on = !(portB & (1 << 2));
        if (m_state->keyUnlocked()) {
            if (on) {
                bool rcActive = !(portB & (1 << 1));
                if (!rcActive)
                    m_previousFlightMode = m_state->flightMode();
                m_state->storePreviousFlightMode(m_previousFlightMode);
                m_state->sendFlightMode("RTL");
            } else {
                bool rcActive = !(portB & (1 << 1));
                if (rcActive)
                    m_state->sendFlightMode("GUIDED");
                else
                    m_state->sendFlightMode(m_previousFlightMode.isEmpty() ? "LOITER" : m_previousFlightMode);
            }
        }
    }

    // Payload arm/fire state changes → update LEDs
    if ((changedB & ((1 << 6) | (1 << 7))) || (changedA & ((1 << 6) | (1 << 7))))
        updatePayloadLeds();
}

void PicoLink::handleButtonPress(uint16_t buttonId)
{
    switch (buttonId) {
    case 3: // SW2_1 Crosshair
        m_state->setCrosshairActive(!m_state->crosshairActive());
        sendMcpLed(0x01, m_state->crosshairActive() ? 0x01 : 0x00);
        break;
    case 4: // SW2_2 Button 1
        sendMcpLed(0x02, 0x02);
        break;
    case 5: // SW2_3 Button 2
        sendMcpLed(0x04, 0x04);
        break;
    }
}

void PicoLink::handleButtonRelease(uint16_t buttonId)
{
    switch (buttonId) {
    case 4: // SW2_2 Button 1
        sendMcpLed(0x02, 0x00);
        break;
    case 5: // SW2_3 Button 2
        sendMcpLed(0x04, 0x00);
        break;
    }
}

void PicoLink::sendLedCmd(uint8_t chain, uint8_t id,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t anim)
{
    uint8_t payload[6] = { chain, id, r, g, b, anim };
    sendFrame(PROTO_TYPE_LED, payload, 6);
}

void PicoLink::sendMcpLed(uint8_t mask, uint8_t state)
{
    uint8_t payload[3] = { 0x02, mask, state };
    sendFrame(PROTO_TYPE_LED, payload, 3);
}

void PicoLink::updatePayloadLeds()
{
    bool pay1Arm  = m_state->sw3Pay1Arm();
    bool pay1Fire = m_state->sw3Pay1Fire();
    bool pay2Arm  = m_state->sw3Pay2Arm();
    bool pay2Fire = m_state->sw3Pay2Fire();

    // PAY1 — WS2811 button 0
    if (pay1Arm && pay1Fire)
        sendLedCmd(0x01, 0, 255, 0, 0, 1);     // solid red
    else if (pay1Arm)
        sendLedCmd(0x01, 0, 255, 100, 0, 2);   // blink slow orange
    else
        sendLedCmd(0x01, 0, 0, 0, 0, 0);       // off

    // PAY2 — WS2811 button 1
    if (pay2Arm && pay2Fire)
        sendLedCmd(0x01, 1, 255, 0, 0, 1);     // solid red
    else if (pay2Arm)
        sendLedCmd(0x01, 1, 255, 100, 0, 2);   // blink slow orange
    else
        sendLedCmd(0x01, 1, 0, 0, 0, 0);       // off
}

void PicoLink::onWorklightChanged(bool on, const QColor &color)
{
    uint8_t r = on ? static_cast<uint8_t>(color.red())   : 0;
    uint8_t g = on ? static_cast<uint8_t>(color.green()) : 0;
    uint8_t b = on ? static_cast<uint8_t>(color.blue())  : 0;
    uint8_t anim = on ? 1 : 0;  // 1 = solid on, 0 = off

    // SK6812 LEDs 70-92 are the worklights (chain 0x00)
    for (uint8_t id = 70; id <= 92; ++id)
        sendLedCmd(0x00, id, r, g, b, anim);
}
