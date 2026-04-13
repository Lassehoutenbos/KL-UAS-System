#pragma once
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>
#include "gcsstate.h"

class PicoLink : public QObject {
    Q_OBJECT
public:
    explicit PicoLink(GCSState *state, QObject *parent = nullptr);
    void start();

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void sendHeartbeat();
    void readCpuTemp();
    void readSystemStats();
    void retryConnect();
    void onBrightnessChanged(int screenL, int screenR, int led, int tft, int btnLeds);
    void onPeriphCmd(int address, int cmd, const QByteArray &payload);
    void onTftScreen(int mode);
    void onTftPeriphDetail(int address);

private:
    void openPort();
    void processPacket(uint8_t type, const uint8_t *payload, int len);
    void handleAdcPacket(const uint8_t *payload, int len);
    void handleDigitalPacket(const uint8_t *payload, int len);
    void handleHeartbeatPacket(const uint8_t *payload, int len);
    void handleAlsPacket(const uint8_t *payload, int len);
    void handleEventPacket(const uint8_t *payload, int len);
    void handlePeriphDataPacket(const uint8_t *payload, int len);
    void handlePeriphStatePacket(const uint8_t *payload, int len);
    void handleSwitchLogic(uint8_t portA, uint8_t portB);
    void handleButtonPress(uint16_t buttonId);
    void handleButtonRelease(uint16_t buttonId);
    void sendLedCmd(uint8_t chain, uint8_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t anim);
    void sendMcpLed(uint8_t mask, uint8_t state);
    void updatePayloadLeds();

    int  buildFrame(uint8_t *buf, int bufSize, uint8_t type,
                    const uint8_t *payload, uint16_t payloadLen);
    void sendFrame(uint8_t type, const uint8_t *payload, uint16_t payloadLen);
    void sendWarningsToPane();
    void updateTftMode();

    double ntcTocelsius(uint16_t rawAdc) const;
    double adcToVolts(uint16_t raw, double divider) const;
    int    batteryPercent(double voltage) const;

    GCSState    *m_state;
    QSerialPort  m_serial;
    QByteArray   m_rxBuf;
    QTimer       m_heartbeatTimer;
    QTimer       m_cpuTempTimer;
    QTimer       m_statsTimer;
    QTimer       m_retryTimer;

    QDateTime    m_lastHeartbeatSent;
    QDateTime    m_lastHeartbeatRecv;
    uint8_t      m_heartbeatSeqTx = 0;
    bool         m_connected      = false;
    int          m_lastTftMode    = -1;
    uint8_t      m_lastPortA      = 0xFF;
    uint8_t      m_lastPortB      = 0xFF;
    bool         m_initialSyncDone = false;
    QString      m_previousFlightMode;

    static constexpr double ADC_VREF      = 3.3;
    static constexpr double ADC_MAX       = 4095.0;
    static constexpr double BAT_DIVIDER   = 8.021;
    static constexpr double EXT_DIVIDER   = 8.021;
    static constexpr double NTC_R0        = 10000.0;
    static constexpr double NTC_BETA      = 3950.0;
    static constexpr double NTC_T0        = 298.15;
    static constexpr double NTC_RFIXED    = 10000.0;
    static constexpr double BAT_FULL_V    = 25.2;
    static constexpr double BAT_EMPTY_V   = 19.8;
};
