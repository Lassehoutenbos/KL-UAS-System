#include "HIDHandler.h"

HIDHandler::HIDHandler(QObject *parent) : QObject(parent) {
    hid_init();
    connect(&timer, &QTimer::timeout, this, &HIDHandler::poll);
}

HIDHandler::~HIDHandler() {
    if (device)
        hid_close(device);
    hid_exit();
}

bool HIDHandler::openFirst() {
    hid_device_info *info = hid_enumerate(0x0, 0x0);
    if (!info)
        return false;
    device = hid_open_path(info->path);
    hid_free_enumeration(info);
    if (!device)
        return false;
    timer.start(100);
    return true;
}

void HIDHandler::poll() {
    if (!device)
        return;
    unsigned char buf[64];
    int res = hid_read(device, buf, sizeof(buf));
    if (res > 0)
        emit report(QByteArray(reinterpret_cast<char *>(buf), res));
}
