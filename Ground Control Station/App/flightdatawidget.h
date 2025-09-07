#ifndef FLIGHTDATAWIDGET_H
#define FLIGHTDATAWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QLCDNumber>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

class FlightDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FlightDataWidget(QWidget *parent = nullptr);
    
    void updateFlightData(double altitude, double speed, double roll, double pitch, 
                         double yaw, int battery, int satellites, int signal);
    void updateGPSData(double lat, double lon, double altitude);
    void updateSystemStatus(const QString &status);

private slots:
    void clearCharts();
    void exportData();
    void toggleRecording();

private:
    // UI Components
    QVBoxLayout *mainLayout;
    
    // Display widgets for real-time data
    QLabel *altitudeDisplay;
    QLabel *speedDisplay;
    QLabel *rollDisplay;
    QLabel *pitchDisplay;
    QLabel *yawDisplay;
    QLabel *batteryDisplay;
    QLabel *satelliteDisplay;
    QLabel *signalDisplay;
    QLabel *latitudeDisplay;
    QLabel *longitudeDisplay;
    QLabel *gpsAltitudeDisplay;
    
    // Statistics displays
    QLabel *maxAltitudeDisplay;
    QLabel *maxSpeedDisplay;
    QLabel *distanceDisplay;
    QLabel *flightTimeDisplay;
    
    // Progress bars
    QProgressBar *batteryProgress;
    QProgressBar *signalProgress;
    
    // Data logging components
    QTableWidget *dataTable;
    QPushButton *clearButton;
    QPushButton *exportButton;
    QPushButton *recordingButton;
    QLabel *recordingStatus;
    QTextEdit *dataDisplay;
    QTimer *dataTimer;
    
    // Current flight data
    bool isRecording;
    double currentAltitude;
    double currentSpeed;
    double currentRoll;
    double currentPitch;
    double currentYaw;
    int currentBattery;
    int currentSatellites;
    int currentSignal;
    double currentLatitude;
    double currentLongitude;
    
    // Flight statistics
    double maxAltitude;
    double maxSpeed;
    double totalDistance;
    double startTime;
    double homeLatitude;
    double homeLongitude;
    double flightStartTime;
    
    void setupUI();
    void setupDataLogging();
    void addDataPoint();
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    QString formatTime(double seconds);
};

#endif // FLIGHTDATAWIDGET_H
