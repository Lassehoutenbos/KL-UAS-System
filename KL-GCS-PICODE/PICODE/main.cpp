#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "backend/gcsstate.h"
#include "backend/picolink.h"
#include "backend/mavlinklink.h"

static void messageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QFile f("C:/Users/houte/Desktop/picode_log.txt");
    f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    QTextStream out(&f);
    out << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << " " << msg << "\n";
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandler);

    // qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);

    qmlRegisterSingletonInstance("PICODE", 1, 0, "GCSState", GCSState::instance());

    PicoLink  *pico = new PicoLink(GCSState::instance(), &app);
    MavlinkLink *mav = new MavlinkLink(GCSState::instance(), &app);
    pico->start();
    mav->start();

    QList<QScreen *> screens = app.screens();

    QQmlApplicationEngine engine0;
    engine0.rootContext()->setContextProperty("screenIndex", 0);
    engine0.rootContext()->setContextProperty("initialPage", "dash");
    engine0.loadFromModule("PICODE", "Main");

    QQmlApplicationEngine *engine1 = nullptr;
    if (screens.size() >= 2) {
        engine1 = new QQmlApplicationEngine(&app);
        engine1->rootContext()->setContextProperty("screenIndex", 1);
        engine1->rootContext()->setContextProperty("initialPage", "drone");
        engine1->loadFromModule("PICODE", "Main");
    }

    return app.exec();
}
