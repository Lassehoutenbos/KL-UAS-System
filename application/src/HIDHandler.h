#pragma once
#include <QObject>
#include <hidapi/hidapi.h>
#include <QTimer>

class HIDHandler : public QObject {
    Q_OBJECT
public:
    explicit HIDHandler(QObject *parent = nullptr);
    ~HIDHandler();
    bool openFirst();
signals:
    void report(const QByteArray &data);
private slots:
    void poll();
private:
    hid_device *device = nullptr;
    QTimer timer;
};
