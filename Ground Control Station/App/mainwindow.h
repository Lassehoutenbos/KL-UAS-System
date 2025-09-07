#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QLCDNumber>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QFrame>
#include <QDial>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPixmap>
#include <QDateTime>
#include "mapwidget.h"
#include "flightdatawidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateTelemetry();
    void connectToUAV();
    void disconnectFromUAV();
    void emergencyStop();
    void armDisarm();
    void takeOff();
    void land();
    void returnToHome();
    void onModeChanged();
    void updateArtificialHorizon();
    void onWaypointAdded(double lat, double lon);
    void onWaypointRemoved(int index);
    void showFlightPlanDialog();
    void showCalibrationDialog();
    void showParameterDialog();
    void showLogBrowser();

private:
    Ui::MainWindow *ui;
    
    // Timers
    QTimer *telemetryTimer;
    QTimer *horizonTimer;
    
    // Connection status
    bool isConnected;
    bool isArmed;
    
    // Main UI Components
    QTabWidget *mainTabs;
    QSplitter *mainSplitter;
    
    // Control Panel Components
    QWidget *controlPanel;
    QLabel *connectionStatusLabel;
    QLabel *batteryLabel;
    QLabel *gpsLabel;
    QLabel *altitudeLabel;
    QLabel *speedLabel;
    QLabel *headingLabel;
    QPushButton *connectBtn;
    QPushButton *armBtn;
    QPushButton *takeOffBtn;
    QPushButton *landBtn;
    QPushButton *rthBtn;
    QPushButton *emergencyBtn;
    QComboBox *flightModeCombo;
    QTextEdit *consoleOutput;
    QLCDNumber *batteryLCD;
    QProgressBar *signalStrengthBar;
    QGraphicsView *artificialHorizonView;
    QGraphicsScene *horizonScene;
    QGraphicsEllipseItem *horizonCircle;
    QGraphicsTextItem *rollText;
    QGraphicsTextItem *pitchText;
    
    // Tab widgets
    MapWidget *mapWidget;
    FlightDataWidget *flightDataWidget;
    QWidget *missionPlanWidget;
    QWidget *settingsWidget;
    
    // Settings tabs
    QTabWidget *settingsTabs;
    QWidget *connectionSettingsTab;
    QWidget *flightSettingsTab;
    QWidget *displaySettingsTab;
    
    // Mission planning components
    QTableWidget *waypointTable;
    QPushButton *addWaypointBtn;
    QPushButton *removeWaypointBtn;
    QPushButton *clearWaypointsBtn;
    QPushButton *uploadMissionBtn;
    QPushButton *downloadMissionBtn;
    QPushButton *startMissionBtn;
    
    // Telemetry data (simulated)
    double latitude;
    double longitude;
    double altitude;
    double speed;
    double heading;
    double roll;
    double pitch;
    double yaw;
    int batteryPercentage;
    int satelliteCount;
    int signalStrength;
    
    void setupUI();
    void setupMainTabs();
    void setupControlPanel();
    void setupFlightPlanTab();
    void setupMapTab();
    void setupFlightDataTab();
    void setupSettingsTab();
    QGroupBox* setupTelemetryPanel();
    void setupArtificialHorizon();
    void setupMenuBar();
    void setupStatusBar();
    void updateConnectionStatus();
    void logMessage(const QString &message);
    void simulateTelemetryData();
    void updateWaypointTable();
};
#endif // MAINWINDOW_H
