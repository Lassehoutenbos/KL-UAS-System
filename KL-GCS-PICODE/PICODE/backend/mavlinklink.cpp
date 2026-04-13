#include "mavlinklink.h"
#include <QDateTime>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QtMath>
#include <cstring>

MavlinkLink::MavlinkLink(GCSState *state, QObject *parent)
    : QObject(parent), m_state(state), m_udp(nullptr)
{
    m_serial.setPortName("/dev/ttyUSB0");
    m_serial.setBaudRate(QSerialPort::Baud57600);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setParity(QSerialPort::NoParity);

    connect(&m_serial, &QSerialPort::readyRead, this, &MavlinkLink::onSerialReadyRead);
    connect(&m_serial, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, &MavlinkLink::onSerialError);

    m_autoDetectTimer.setInterval(500);
    connect(&m_autoDetectTimer, &QTimer::timeout, this, &MavlinkLink::retryAutoDetect);

    m_heartbeatTimer.setInterval(HEARTBEAT_TIMEOUT_MS);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &MavlinkLink::heartbeatTimeout);

    connect(m_state, &GCSState::cmdSetMode, this, &MavlinkLink::onModeChangeRequested);
    connect(m_state, &GCSState::cmdArmDisarm, this, &MavlinkLink::onArmDisarmRequested);
    connect(m_state, &GCSState::cmdMissionUpload, this, &MavlinkLink::onMissionUploadRequested);
    connect(m_state, &GCSState::cmdMissionDownload, this, &MavlinkLink::onMissionDownloadRequested);
    connect(m_state, &GCSState::cmdParamWrite, this, &MavlinkLink::onParamWriteRequested);
    connect(m_state, &GCSState::cmdParamRefresh, this, &MavlinkLink::onParamRefreshRequested);
}

void MavlinkLink::start()
{
    trySerialPort();
    m_autoDetectTimer.start();
    m_heartbeatTimer.start();
}

void MavlinkLink::trySerialPort()
{
    if (m_serial.open(QIODevice::ReadWrite)) {
        m_transport = Serial;
        m_connected = true;
        m_autoDetectTimer.stop();
    } else {
        m_transport = None;
        m_connected = false;
    }
}

void MavlinkLink::tryUdpPort()
{
    if (!m_udp) {
        m_udp = new QUdpSocket(this);
        connect(m_udp, &QUdpSocket::readyRead, this, &MavlinkLink::onUdpReadyRead);
    }

    bool bound = m_udp->bind(QHostAddress::Any, 14550);
    if (bound) {
        m_transport = Udp;
        m_connected = true;
        m_autoDetectTimer.stop();
    }
}

void MavlinkLink::retryAutoDetect()
{
    if (!m_connected) {
        if (m_transport == None) {
            trySerialPort();
            if (!m_connected)
                tryUdpPort();
        }
    }
}

void MavlinkLink::onSerialReadyRead()
{
    m_rxBuf.append(m_serial.readAll());
    parseData(m_rxBuf);
}

void MavlinkLink::onUdpReadyRead()
{
    if (!m_udp) return;

    while (m_udp->hasPendingDatagrams()) {
        QByteArray datagram = m_udp->receiveDatagram().data();
        m_rxBuf.append(datagram);
    }
    parseData(m_rxBuf);
}

void MavlinkLink::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        if (m_serial.isOpen())
            m_serial.close();
        m_connected = false;
        m_state->updateMavlinkLink(false, 0, 0);
    }
}

void MavlinkLink::heartbeatTimeout()
{
    if (m_connected) {
        m_connected = false;
        m_state->updateMavlinkLink(false, 0, 0);
    }
}

void MavlinkLink::parseData(const QByteArray &data)
{
    for (int i = 0; i < data.size(); ++i) {
        if ((uint8_t)data[i] == MAVLINK_STX) {
            if (i + 10 <= data.size()) {
                uint8_t len = (uint8_t)data[i + 1];
                if (i + 10 + len <= data.size()) {
                    i += 10 + len;
                }
            }
        }
    }
}

void MavlinkLink::handleHeartbeat(const uint8_t *data)
{
    m_connected = true;
    m_heartbeatTimer.start();
    m_state->updateMavlinkLink(true, m_state->mavlinkRssi(), m_state->mavlinkSnr());
}

void MavlinkLink::handleVfrHud(const uint8_t *data)
{
}

void MavlinkLink::handleAttitude(const uint8_t *data)
{
}

void MavlinkLink::handleGpsRawInt(const uint8_t *data)
{
}

void MavlinkLink::handleSysStatus(const uint8_t *data)
{
}

void MavlinkLink::handleRadioStatus(const uint8_t *data)
{
}

void MavlinkLink::handleStatustext(const uint8_t *data)
{
}

void MavlinkLink::handleParamValue(const uint8_t *data)
{
}

void MavlinkLink::handleMissionCount(const uint8_t *data)
{
}

void MavlinkLink::handleMissionItemInt(const uint8_t *data)
{
}

void MavlinkLink::handleGlobalPositionInt(const uint8_t *data)
{
}

uint8_t MavlinkLink::calculateChecksum(const uint8_t *data, int len)
{
    uint8_t checksum = 0;
    for (int i = 0; i < len; ++i)
        checksum ^= data[i];
    return checksum;
}

int MavlinkLink::buildMavlinkFrame(uint8_t *buf, int bufSize, uint8_t msgId,
                                    const uint8_t *payload, uint8_t payloadLen)
{
    if (bufSize < (int)(10 + payloadLen))
        return -1;

    buf[0] = MAVLINK_STX;
    buf[1] = payloadLen;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = msgId;
    if (payloadLen > 0)
        memcpy(&buf[5], payload, payloadLen);

    uint8_t cksum = calculateChecksum(&buf[1], 3 + payloadLen);
    buf[5 + payloadLen] = cksum;
    buf[6 + payloadLen] = 0;

    return 7 + payloadLen;
}

void MavlinkLink::sendMavlinkFrame(uint8_t msgId, const uint8_t *payload, uint8_t payloadLen)
{
    if (!m_connected) return;

    uint8_t buf[256];
    int frameLen = buildMavlinkFrame(buf, sizeof(buf), msgId, payload, payloadLen);
    if (frameLen > 0) {
        if (m_transport == Serial)
            m_serial.write((const char *)buf, frameLen);
        else if (m_transport == Udp && m_udp)
            m_udp->writeDatagram((const char *)buf, frameLen, QHostAddress::LocalHost, 14550);
    }
}

void MavlinkLink::onModeChangeRequested(const QString &mode)
{
}

void MavlinkLink::onArmDisarmRequested(bool arm)
{
}

void MavlinkLink::onMissionUploadRequested(const QVariantList &wps)
{
}

void MavlinkLink::onMissionDownloadRequested()
{
}

void MavlinkLink::onParamWriteRequested(const QString &name, double value)
{
}

void MavlinkLink::onParamRefreshRequested()
{
}
