#pragma once
#include <QObject>
#include <QSerialPort>

class SerialHandler : public QObject {
    Q_OBJECT
public:
    explicit SerialHandler(QObject *parent = nullptr);
    void open(const QString &portName, int baudRate = 115200);
signals:
    void received(const QByteArray &data);
private slots:
    void handleReadyRead();
private:
    QSerialPort serial;
};
