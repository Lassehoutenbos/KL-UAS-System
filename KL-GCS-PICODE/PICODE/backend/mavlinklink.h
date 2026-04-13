#pragma once
#include <QObject>
#include <QSerialPort>
#include <QUdpSocket>
#include <QTimer>
#include <QVariantList>
#include "gcsstate.h"

class MavlinkLink : public QObject {
    Q_OBJECT
public:
    explicit MavlinkLink(GCSState *state, QObject *parent = nullptr);
    void start();

private slots:
    void retryAutoDetect();
    void onSerialReadyRead();
    void onUdpReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void heartbeatTimeout();
    void onModeChangeRequested(const QString &mode);
    void onArmDisarmRequested(bool arm);
    void onMissionUploadRequested(const QVariantList &wps);
    void onMissionDownloadRequested();
    void onParamWriteRequested(const QString &name, double value);
    void onParamRefreshRequested();

private:
    void trySerialPort();
    void tryUdpPort();
    void parseData(const QByteArray &data);
    void handleHeartbeat(const uint8_t *data);
    void handleVfrHud(const uint8_t *data);
    void handleAttitude(const uint8_t *data);
    void handleGpsRawInt(const uint8_t *data);
    void handleSysStatus(const uint8_t *data);
    void handleRadioStatus(const uint8_t *data);
    void handleStatustext(const uint8_t *data);
    void handleParamValue(const uint8_t *data);
    void handleMissionCount(const uint8_t *data);
    void handleMissionItemInt(const uint8_t *data);
    void handleGlobalPositionInt(const uint8_t *data);

    uint8_t calculateChecksum(const uint8_t *data, int len);
    int buildMavlinkFrame(uint8_t *buf, int bufSize, uint8_t msgId,
                          const uint8_t *payload, uint8_t payloadLen);
    void sendMavlinkFrame(uint8_t msgId, const uint8_t *payload, uint8_t payloadLen);

    GCSState      *m_state;
    QSerialPort    m_serial;
    QUdpSocket    *m_udp = nullptr;
    QByteArray     m_rxBuf;
    QTimer         m_autoDetectTimer;
    QTimer         m_heartbeatTimer;

    enum TransportMode { None, Serial, Udp };
    TransportMode m_transport = None;
    bool           m_connected = false;
    bool           m_paramBuffering = false;
    QVariantList   m_paramBuffer;
    int            m_paramExpectedCount = 0;
    int            m_missionItemCount = 0;
    int            m_missionItemIndex = 0;
    QVariantList   m_missionBuffer;

    static constexpr uint8_t MAVLINK_STX = 0xFD;
    static constexpr int HEARTBEAT_TIMEOUT_MS = 3000;
};
