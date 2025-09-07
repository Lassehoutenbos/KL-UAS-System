#include "MainWindow.h"
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    log = new QTextEdit(this);
    log->setReadOnly(true);
    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->addWidget(log);
    setCentralWidget(central);

    connect(&serial, &SerialHandler::received, this, [this](const QByteArray &data){
        log->append(QString::fromUtf8("Serial: ") + QString::fromUtf8(data));
    });

    connect(&hid, &HIDHandler::report, this, [this](const QByteArray &data){
        log->append(QString::fromUtf8("HID: ") + data.toHex(' '));
    });

    serial.open(QStringLiteral("/dev/ttyUSB0"));
    hid.openFirst();
}
