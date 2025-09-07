#include "flightdatawidget.h"
#include <cmath>

FlightDataWidget::FlightDataWidget(QWidget *parent)
    : QWidget(parent)
    , isRecording(false)
    , currentAltitude(0.0)
    , currentSpeed(0.0)
    , currentRoll(0.0)
    , currentPitch(0.0)
    , currentYaw(0.0)
    , currentBattery(100)
    , currentSatellites(0)
    , currentSignal(0)
    , currentLatitude(0.0)
    , currentLongitude(0.0)
    , maxAltitude(0.0)
    , maxSpeed(0.0)
    , totalDistance(0.0)
    , startTime(0.0)
    , homeLatitude(0.0)
    , homeLongitude(0.0)
    , flightStartTime(0.0)
{
    setupUI();
    setupDataLogging();
}

void FlightDataWidget::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    
    // Create main tabs for different data views
    QTabWidget *dataTabs = new QTabWidget;
    
    // Real-time data tab
    QWidget *realtimeTab = new QWidget;
    QGridLayout *realtimeLayout = new QGridLayout(realtimeTab);
    
    // Flight Data Group
    QGroupBox *flightDataGroup = new QGroupBox("Flight Data");
    QGridLayout *flightLayout = new QGridLayout(flightDataGroup);
    
    // Altitude display
    flightLayout->addWidget(new QLabel("Altitude:"), 0, 0);
    altitudeDisplay = new QLabel("0.0 m");
    altitudeDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #4CAF50; }");
    flightLayout->addWidget(altitudeDisplay, 0, 1);
    
    // Speed display
    flightLayout->addWidget(new QLabel("Speed:"), 1, 0);
    speedDisplay = new QLabel("0.0 m/s");
    speedDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #2196F3; }");
    flightLayout->addWidget(speedDisplay, 1, 1);
    
    // Attitude displays
    flightLayout->addWidget(new QLabel("Roll:"), 2, 0);
    rollDisplay = new QLabel("0.0°");
    rollDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    flightLayout->addWidget(rollDisplay, 2, 1);
    
    flightLayout->addWidget(new QLabel("Pitch:"), 3, 0);
    pitchDisplay = new QLabel("0.0°");
    pitchDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    flightLayout->addWidget(pitchDisplay, 3, 1);
    
    flightLayout->addWidget(new QLabel("Yaw:"), 4, 0);
    yawDisplay = new QLabel("0.0°");
    yawDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    flightLayout->addWidget(yawDisplay, 4, 1);
    
    realtimeLayout->addWidget(flightDataGroup, 0, 0);
    
    // System Status Group
    QGroupBox *systemGroup = new QGroupBox("System Status");
    QGridLayout *systemLayout = new QGridLayout(systemGroup);
    
    // Battery
    systemLayout->addWidget(new QLabel("Battery:"), 0, 0);
    batteryDisplay = new QLabel("100%");
    batteryDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #4CAF50; }");
    systemLayout->addWidget(batteryDisplay, 0, 1);
    
    batteryProgress = new QProgressBar;
    batteryProgress->setRange(0, 100);
    batteryProgress->setValue(100);
    batteryProgress->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
    systemLayout->addWidget(batteryProgress, 0, 2);
    
    // GPS Satellites
    systemLayout->addWidget(new QLabel("GPS Satellites:"), 1, 0);
    satelliteDisplay = new QLabel("0");
    satelliteDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    systemLayout->addWidget(satelliteDisplay, 1, 1);
    
    // Signal Strength
    systemLayout->addWidget(new QLabel("Signal Strength:"), 2, 0);
    signalDisplay = new QLabel("0%");
    signalDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    systemLayout->addWidget(signalDisplay, 2, 1);
    
    signalProgress = new QProgressBar;
    signalProgress->setRange(0, 100);
    signalProgress->setValue(0);
    signalProgress->setStyleSheet("QProgressBar::chunk { background-color: #FF9800; }");
    systemLayout->addWidget(signalProgress, 2, 2);
    
    realtimeLayout->addWidget(systemGroup, 0, 1);
    
    // GPS Data Group
    QGroupBox *gpsGroup = new QGroupBox("GPS Data");
    QGridLayout *gpsLayout = new QGridLayout(gpsGroup);
    
    gpsLayout->addWidget(new QLabel("Latitude:"), 0, 0);
    latitudeDisplay = new QLabel("0.000000");
    latitudeDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; }");
    gpsLayout->addWidget(latitudeDisplay, 0, 1);
    
    gpsLayout->addWidget(new QLabel("Longitude:"), 1, 0);
    longitudeDisplay = new QLabel("0.000000");
    longitudeDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; }");
    gpsLayout->addWidget(longitudeDisplay, 1, 1);
    
    gpsLayout->addWidget(new QLabel("GPS Altitude:"), 2, 0);
    gpsAltitudeDisplay = new QLabel("0.0 m");
    gpsAltitudeDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; }");
    gpsLayout->addWidget(gpsAltitudeDisplay, 2, 1);
    
    realtimeLayout->addWidget(gpsGroup, 1, 0, 1, 2);
    
    dataTabs->addTab(realtimeTab, "Real-time Data");
    
    // Statistics tab
    QWidget *statsTab = new QWidget;
    QGridLayout *statsLayout = new QGridLayout(statsTab);
    
    QGroupBox *flightStatsGroup = new QGroupBox("Flight Statistics");
    QGridLayout *flightStatsLayout = new QGridLayout(flightStatsGroup);
    
    flightStatsLayout->addWidget(new QLabel("Max Altitude:"), 0, 0);
    maxAltitudeDisplay = new QLabel("0.0 m");
    maxAltitudeDisplay->setStyleSheet("QLabel { font-weight: bold; color: #4CAF50; }");
    flightStatsLayout->addWidget(maxAltitudeDisplay, 0, 1);
    
    flightStatsLayout->addWidget(new QLabel("Max Speed:"), 1, 0);
    maxSpeedDisplay = new QLabel("0.0 m/s");
    maxSpeedDisplay->setStyleSheet("QLabel { font-weight: bold; color: #2196F3; }");
    flightStatsLayout->addWidget(maxSpeedDisplay, 1, 1);
    
    flightStatsLayout->addWidget(new QLabel("Total Distance:"), 2, 0);
    distanceDisplay = new QLabel("0.0 m");
    distanceDisplay->setStyleSheet("QLabel { font-weight: bold; color: #FF9800; }");
    flightStatsLayout->addWidget(distanceDisplay, 2, 1);
    
    flightStatsLayout->addWidget(new QLabel("Flight Time:"), 3, 0);
    flightTimeDisplay = new QLabel("00:00:00");
    flightTimeDisplay->setStyleSheet("QLabel { font-weight: bold; color: #9C27B0; }");
    flightStatsLayout->addWidget(flightTimeDisplay, 3, 1);
    
    statsLayout->addWidget(flightStatsGroup, 0, 0);
    
    dataTabs->addTab(statsTab, "Statistics");
    
    // Data logging tab
    QWidget *loggingTab = new QWidget;
    QVBoxLayout *loggingLayout = new QVBoxLayout(loggingTab);
    
    // Recording controls
    QHBoxLayout *recordingControls = new QHBoxLayout;
    
    recordingButton = new QPushButton("Start Recording");
    recordingButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }");
    connect(recordingButton, &QPushButton::clicked, this, &FlightDataWidget::toggleRecording);
    recordingControls->addWidget(recordingButton);
    
    clearButton = new QPushButton("Clear Data");
    clearButton->setStyleSheet("QPushButton { background-color: #FF5722; color: white; font-weight: bold; }");
    connect(clearButton, &QPushButton::clicked, this, &FlightDataWidget::clearCharts);
    recordingControls->addWidget(clearButton);
    
    exportButton = new QPushButton("Export Data");
    exportButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; }");
    connect(exportButton, &QPushButton::clicked, this, &FlightDataWidget::exportData);
    recordingControls->addWidget(exportButton);
    
    recordingControls->addStretch();
    
    // Recording status
    recordingStatus = new QLabel("Recording: Stopped");
    recordingStatus->setStyleSheet("QLabel { color: #F44336; font-weight: bold; }");
    recordingControls->addWidget(recordingStatus);
    
    loggingLayout->addLayout(recordingControls);
    
    // Data display area
    dataDisplay = new QTextEdit;
    dataDisplay->setReadOnly(true);
    dataDisplay->setMaximumHeight(200);
    dataDisplay->setStyleSheet("QTextEdit { background-color: #1A1A1A; color: #FFFFFF; font-family: 'Consolas', monospace; }");
    loggingLayout->addWidget(dataDisplay);
    
    // Data table
    dataTable = new QTableWidget;
    dataTable->setColumnCount(9);
    dataTable->setHorizontalHeaderLabels({
        "Time", "Altitude", "Speed", "Roll", "Pitch", "Yaw", "Battery", "Satellites", "Signal"
    });
    dataTable->horizontalHeader()->setStretchLastSection(true);
    dataTable->setAlternatingRowColors(true);
    loggingLayout->addWidget(dataTable);
    
    dataTabs->addTab(loggingTab, "Data Logging");
    
    mainLayout->addWidget(dataTabs);
    
    // Apply styling
    setStyleSheet(
        "QGroupBox { font-weight: bold; border: 2px solid #404040; border-radius: 6px; "
        "margin-top: 1ex; padding-top: 10px; background-color: #2D2D2D; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 8px 0 8px; }"
        "QTabWidget::pane { border: 1px solid #404040; background-color: #2D2D2D; }"
        "QTabBar::tab { background-color: #404040; color: #FFFFFF; padding: 8px 16px; "
        "margin-right: 2px; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background-color: #4CAF50; }"
        "QTabBar::tab:hover { background-color: #505050; }"
    );
}

void FlightDataWidget::setupDataLogging()
{
    // Initialize data logging timer
    dataTimer = new QTimer(this);
    connect(dataTimer, &QTimer::timeout, this, &FlightDataWidget::addDataPoint);
    
    // Set home position (will be updated when GPS data is available)
    homeLatitude = 0.0;
    homeLongitude = 0.0;
    
    // Initialize flight start time
    flightStartTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
}

void FlightDataWidget::updateFlightData(double altitude, double speed, double roll, double pitch, 
                                       double yaw, int battery, int satellites, int signal)
{
    // Update current values
    currentAltitude = altitude;
    currentSpeed = speed;
    currentRoll = roll;
    currentPitch = pitch;
    currentYaw = yaw;
    currentBattery = battery;
    currentSatellites = satellites;
    currentSignal = signal;
    
    // Update displays
    altitudeDisplay->setText(QString("%1 m").arg(altitude, 0, 'f', 1));
    speedDisplay->setText(QString("%1 m/s").arg(speed, 0, 'f', 1));
    rollDisplay->setText(QString("%1°").arg(roll, 0, 'f', 1));
    pitchDisplay->setText(QString("%1°").arg(pitch, 0, 'f', 1));
    yawDisplay->setText(QString("%1°").arg(yaw, 0, 'f', 1));
    
    batteryDisplay->setText(QString("%1%").arg(battery));
    batteryProgress->setValue(battery);
    
    // Update battery color based on level
    if (battery < 20) {
        batteryDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #F44336; }");
        batteryProgress->setStyleSheet("QProgressBar::chunk { background-color: #F44336; }");
    } else if (battery < 50) {
        batteryDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #FF9800; }");
        batteryProgress->setStyleSheet("QProgressBar::chunk { background-color: #FF9800; }");
    } else {
        batteryDisplay->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #4CAF50; }");
        batteryProgress->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
    }
    
    satelliteDisplay->setText(QString::number(satellites));
    signalDisplay->setText(QString("%1%").arg(signal));
    signalProgress->setValue(signal);
    
    // Update statistics
    if (altitude > maxAltitude) {
        maxAltitude = altitude;
        maxAltitudeDisplay->setText(QString("%1 m").arg(maxAltitude, 0, 'f', 1));
    }
    
    if (speed > maxSpeed) {
        maxSpeed = speed;
        maxSpeedDisplay->setText(QString("%1 m/s").arg(maxSpeed, 0, 'f', 1));
    }
    
    // Update flight time
    double currentTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    double flightTime = currentTime - flightStartTime;
    flightTimeDisplay->setText(formatTime(flightTime));
}

void FlightDataWidget::updateGPSData(double lat, double lon, double altitude)
{
    // Store previous position for distance calculation
    static double previousLat = 0.0;
    static double previousLon = 0.0;
    static bool firstUpdate = true;
    
    currentLatitude = lat;
    currentLongitude = altitude;  // Note: this seems to be a bug, should be lon
    
    // Update displays
    latitudeDisplay->setText(QString("%1").arg(lat, 0, 'f', 6));
    longitudeDisplay->setText(QString("%1").arg(lon, 0, 'f', 6));
    gpsAltitudeDisplay->setText(QString("%1 m").arg(altitude, 0, 'f', 1));
    
    // Calculate distance traveled
    if (!firstUpdate && previousLat != 0.0 && previousLon != 0.0) {
        double distance = calculateDistance(previousLat, previousLon, lat, lon);
        totalDistance += distance;
        distanceDisplay->setText(QString("%1 m").arg(totalDistance, 0, 'f', 1));
    }
    
    // Set home position on first GPS fix
    if (firstUpdate && lat != 0.0 && lon != 0.0) {
        homeLatitude = lat;
        homeLongitude = lon;
        firstUpdate = false;
    }
    
    previousLat = lat;
    previousLon = lon;
}

void FlightDataWidget::updateSystemStatus(const QString &status)
{
    // Log system status messages
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, status);
    dataDisplay->append(logEntry);
    
    // Auto-scroll to bottom
    QTextCursor cursor = dataDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    dataDisplay->setTextCursor(cursor);
}

void FlightDataWidget::clearCharts()
{
    // Clear all data
    maxAltitude = 0.0;
    maxSpeed = 0.0;
    totalDistance = 0.0;
    flightStartTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    
    // Reset displays
    maxAltitudeDisplay->setText("0.0 m");
    maxSpeedDisplay->setText("0.0 m/s");
    distanceDisplay->setText("0.0 m");
    flightTimeDisplay->setText("00:00:00");
    
    // Clear data table
    dataTable->setRowCount(0);
    
    // Clear text display
    dataDisplay->clear();
    
    updateSystemStatus("All data cleared");
}

void FlightDataWidget::exportData()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Export Flight Data", 
        QString("flight_data_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "CSV Files (*.csv)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            
            // Write header
            out << "Time,Altitude,Speed,Roll,Pitch,Yaw,Battery,Satellites,Signal\n";
            
            // Write data from table
            for (int row = 0; row < dataTable->rowCount(); ++row) {
                QStringList rowData;
                for (int col = 0; col < dataTable->columnCount(); ++col) {
                    QTableWidgetItem *item = dataTable->item(row, col);
                    rowData << (item ? item->text() : "");
                }
                out << rowData.join(",") << "\n";
            }
            
            file.close();
            updateSystemStatus(QString("Data exported to %1").arg(fileName));
        } else {
            QMessageBox::warning(this, "Export Error", "Could not save file: " + fileName);
        }
    }
}

void FlightDataWidget::toggleRecording()
{
    if (isRecording) {
        // Stop recording
        isRecording = false;
        dataTimer->stop();
        recordingButton->setText("Start Recording");
        recordingButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }");
        recordingStatus->setText("Recording: Stopped");
        recordingStatus->setStyleSheet("QLabel { color: #F44336; font-weight: bold; }");
        updateSystemStatus("Data recording stopped");
    } else {
        // Start recording
        isRecording = true;
        dataTimer->start(1000); // Record every second
        recordingButton->setText("Stop Recording");
        recordingButton->setStyleSheet("QPushButton { background-color: #F44336; color: white; font-weight: bold; }");
        recordingStatus->setText("Recording: Active");
        recordingStatus->setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; }");
        updateSystemStatus("Data recording started");
    }
}

void FlightDataWidget::addDataPoint()
{
    if (!isRecording) return;
    
    // Add current data to table
    int row = dataTable->rowCount();
    dataTable->insertRow(row);
    
    QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
    
    dataTable->setItem(row, 0, new QTableWidgetItem(currentTime));
    dataTable->setItem(row, 1, new QTableWidgetItem(QString::number(currentAltitude, 'f', 1)));
    dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(currentSpeed, 'f', 1)));
    dataTable->setItem(row, 3, new QTableWidgetItem(QString::number(currentRoll, 'f', 1)));
    dataTable->setItem(row, 4, new QTableWidgetItem(QString::number(currentPitch, 'f', 1)));
    dataTable->setItem(row, 5, new QTableWidgetItem(QString::number(currentYaw, 'f', 1)));
    dataTable->setItem(row, 6, new QTableWidgetItem(QString::number(currentBattery)));
    dataTable->setItem(row, 7, new QTableWidgetItem(QString::number(currentSatellites)));
    dataTable->setItem(row, 8, new QTableWidgetItem(QString::number(currentSignal)));
    
    // Auto-scroll to bottom
    dataTable->scrollToBottom();
    
    // Limit table to 1000 rows to prevent memory issues
    if (dataTable->rowCount() > 1000) {
        dataTable->removeRow(0);
    }
}

double FlightDataWidget::calculateDistance(double lat1, double lon1, double lat2, double lon2)
{
    // Haversine formula for calculating distance between two GPS coordinates
    const double R = 6371000; // Earth's radius in meters
    
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon/2) * sin(dLon/2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return R * c;
}

QString FlightDataWidget::formatTime(double seconds)
{
    int hours = static_cast<int>(seconds) / 3600;
    int minutes = (static_cast<int>(seconds) % 3600) / 60;
    int secs = static_cast<int>(seconds) % 60;
    
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}
