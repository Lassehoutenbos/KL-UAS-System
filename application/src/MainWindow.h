#pragma once
#include <QMainWindow>
#include <QTextEdit>
#include "SerialHandler.h"
#include "HIDHandler.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
private:
    QTextEdit *log;
    SerialHandler serial;
    HIDHandler hid;
};
