#include "SerialHandler.h"

SerialHandler::SerialHandler(QObject *parent) : QObject(parent) {
    connect(&serial, &QSerialPort::readyRead, this, &SerialHandler::handleReadyRead);
}

void SerialHandler::open(const QString &portName, int baudRate) {
    serial.setPortName(portName);
    serial.setBaudRate(baudRate);
    serial.open(QIODevice::ReadWrite);
}

void SerialHandler::handleReadyRead() {
    QByteArray data = serial.readAll();
    emit received(data);
}
