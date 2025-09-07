#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QLCDNumber>
#include <QProgressBar>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QTimer>
#include <QDateTime>
#include <QPen>
#include <QBrush>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <cmath>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isConnected(false)
    , isArmed(false)
    , latitude(52.3676)
    , longitude(4.9041)
    , altitude(0.0)
    , speed(0.0)
    , heading(0.0)
    , roll(0.0)
    , pitch(0.0)
    , yaw(0.0)
    , batteryPercentage(100)
    , satelliteCount(0)
    , signalStrength(0)
{
    ui->setupUi(this);
    setupUI();
    
    // Initialize timers
    telemetryTimer = new QTimer(this);
    horizonTimer = new QTimer(this);
    
    connect(telemetryTimer, &QTimer::timeout, this, &MainWindow::updateTelemetry);
    connect(horizonTimer, &QTimer::timeout, this, &MainWindow::updateArtificialHorizon);
    
    // Start timers
    telemetryTimer->start(100); // Update every 100ms
    horizonTimer->start(50);    // Update horizon every 50ms
    
    logMessage("KL-UAS Ground Control Station initialized");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    setWindowTitle("KL-UAS Ground Control Station");
    setMinimumSize(1400, 900);
    resize(1600, 1000);
    
    // Create main splitter
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    
    // Setup control panel (left side)
    setupControlPanel();
    mainSplitter->addWidget(controlPanel);
    
    // Setup main tabs (right side)
    setupMainTabs();
    mainSplitter->addWidget(mainTabs);
    
    // Set splitter proportions
    mainSplitter->setStretchFactor(0, 0); // Control panel fixed width
    mainSplitter->setStretchFactor(1, 1); // Main area stretches
    mainSplitter->setSizes({400, 1200});
    
    setupMenuBar();
    setupStatusBar();
    
    // Apply refined dark theme with accent color for a cleaner look
    setStyleSheet(
        "QMainWindow { background-color: #1f1f1f; font-family: 'Segoe UI', sans-serif; }"
        "QWidget { background-color: #1f1f1f; color: #f0f0f0; }"
        "QGroupBox { font-weight: bold; color: #f0f0f0; border: 1px solid #3c3c3c; "
        "border-radius: 8px; margin-top: 1ex; padding-top: 10px; background-color: #2b2b2b; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; background-color: #2b2b2b; }"
        "QLabel { color: #f0f0f0; }"
        "QPushButton { border-radius: 4px; padding: 8px 12px; font-weight: bold; border: none; }"
        "QPushButton:hover { background-color: #00adb5; color: #ffffff; }"
        "QPushButton:disabled { background-color: #404040; color: #808080; }"
        "QComboBox { background-color: #404040; color: #f0f0f0; border: 1px solid #606060; "
        "border-radius: 4px; padding: 6px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: none; border: none; }"
        "QProgressBar { background-color: #3c3c3c; border-radius: 4px; text-align: center; }"
        "QProgressBar::chunk { background-color: #00adb5; border-radius: 4px; }"
        "QTextEdit { background-color: #1a1a1a; color: #f0f0f0; border: 1px solid #606060; "
        "border-radius: 4px; font-family: 'Consolas', monospace; }"
        "QTabWidget::pane { border: 1px solid #3c3c3c; background-color: #2b2b2b; }"
        "QTabBar::tab { background-color: #2b2b2b; color: #f0f0f0; padding: 8px 16px; "
        "margin-right: 2px; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background-color: #00adb5; }"
        "QTabBar::tab:hover { background-color: #3c3c3c; }"
        "QTableWidget { background-color: #2b2b2b; alternate-background-color: #353535; "
        "gridline-color: #404040; border: 1px solid #606060; }"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background-color: #00adb5; }"
        "QHeaderView::section { background-color: #2b2b2b; color: #f0f0f0; padding: 6px; "
        "border: none; font-weight: bold; }"
    );
}

void MainWindow::setupMainTabs()
{
    mainTabs = new QTabWidget;
    
    // Setup individual tabs
    setupMapTab();
    setupFlightDataTab();
    setupFlightPlanTab();
    setupSettingsTab();
    
    // Add tabs to widget
    mainTabs->addTab(mapWidget, "🗺️ Map & Mission");
    mainTabs->addTab(flightDataWidget, "📊 Flight Data");
    mainTabs->addTab(missionPlanWidget, "✈️ Mission Plan");
    mainTabs->addTab(settingsWidget, "⚙️ Settings");
    
    // Set default tab
    mainTabs->setCurrentIndex(0);
}

void MainWindow::setupControlPanel()
{
    controlPanel = new QWidget;
    controlPanel->setMaximumWidth(420);
    controlPanel->setMinimumWidth(380);
    
    QVBoxLayout *layout = new QVBoxLayout(controlPanel);
    
    // Connection Group
    QGroupBox *connectionGroup = new QGroupBox("🔗 Connection Status");
    QVBoxLayout *connectionLayout = new QVBoxLayout(connectionGroup);
    
    connectionStatusLabel = new QLabel("Disconnected");
    connectionStatusLabel->setStyleSheet("QLabel { color: #F44336; font-weight: bold; font-size: 14px; }");

    connectBtn = new QPushButton("Connect to UAV");
    connectBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-size: 12px; }");
    connectBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connectBtn->setToolTip("Connect to the UAV");
    connect(connectBtn, &QPushButton::clicked, this, &MainWindow::connectToUAV);
    
    QHBoxLayout *connLayout = new QHBoxLayout;
    connLayout->addWidget(connectionStatusLabel);
    connLayout->addWidget(connectBtn);
    
    connectionLayout->addLayout(connLayout);
    layout->addWidget(connectionGroup);
    
    // Flight Controls Group
    QGroupBox *controlGroup = new QGroupBox("🎮 Flight Controls");
    QGridLayout *controlLayout = new QGridLayout(controlGroup);
    
    armBtn = new QPushButton("ARM");
    armBtn->setStyleSheet("QPushButton { background-color: #FF9800; color: white; }");
    armBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    armBtn->setToolTip("Arm or disarm the UAV motors");
    armBtn->setEnabled(false);
    connect(armBtn, &QPushButton::clicked, this, &MainWindow::armDisarm);

    takeOffBtn = new QPushButton("TAKE OFF");
    takeOffBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");
    takeOffBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    takeOffBtn->setToolTip("Launch the UAV");
    takeOffBtn->setEnabled(false);
    connect(takeOffBtn, &QPushButton::clicked, this, &MainWindow::takeOff);

    landBtn = new QPushButton("LAND");
    landBtn->setStyleSheet("QPushButton { background-color: #9C27B0; color: white; }");
    landBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    landBtn->setToolTip("Land the UAV at the current position");
    landBtn->setEnabled(false);
    connect(landBtn, &QPushButton::clicked, this, &MainWindow::land);

    rthBtn = new QPushButton("RTH");
    rthBtn->setStyleSheet("QPushButton { background-color: #607D8B; color: white; }");
    rthBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    rthBtn->setToolTip("Return to the recorded home location");
    rthBtn->setEnabled(false);
    connect(rthBtn, &QPushButton::clicked, this, &MainWindow::returnToHome);

    emergencyBtn = new QPushButton("🚨 EMERGENCY STOP");
    emergencyBtn->setStyleSheet("QPushButton { background-color: #F44336; color: white; font-weight: bold; }");
    emergencyBtn->setIcon(style()->standardIcon(QStyle::SP_MessageBoxCritical));
    emergencyBtn->setToolTip("Immediately cut power to all motors");
    emergencyBtn->setEnabled(false);
    connect(emergencyBtn, &QPushButton::clicked, this, &MainWindow::emergencyStop);
    
    QLabel *modeLabel = new QLabel("Flight Mode:");
    flightModeCombo = new QComboBox;
    flightModeCombo->addItems({"Manual", "Stabilize", "Altitude Hold", "Position Hold", "RTH", "Auto Mission"});
    flightModeCombo->setToolTip("Select the desired flight mode");
    flightModeCombo->setEnabled(false);
    connect(flightModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onModeChanged);
    
    controlLayout->addWidget(armBtn, 0, 0);
    controlLayout->addWidget(takeOffBtn, 0, 1);
    controlLayout->addWidget(landBtn, 1, 0);
    controlLayout->addWidget(rthBtn, 1, 1);
    controlLayout->addWidget(modeLabel, 2, 0);
    controlLayout->addWidget(flightModeCombo, 2, 1);
    controlLayout->addWidget(emergencyBtn, 3, 0, 1, 2);
    
    layout->addWidget(controlGroup);
    
    // Telemetry Group
    QGroupBox *telemetryGroup = setupTelemetryPanel();
    layout->addWidget(telemetryGroup);
    
    // Artificial Horizon
    setupArtificialHorizon();
    QGroupBox *horizonGroup = new QGroupBox("🛩️ Artificial Horizon");
    QVBoxLayout *horizonLayout = new QVBoxLayout(horizonGroup);
    horizonLayout->addWidget(artificialHorizonView);
    layout->addWidget(horizonGroup);
    
    // Console
    QGroupBox *consoleGroup = new QGroupBox("📝 Console Log");
    QVBoxLayout *consoleLayout = new QVBoxLayout(consoleGroup);
    
    consoleOutput = new QTextEdit;
    consoleOutput->setMaximumHeight(120);
    consoleOutput->setReadOnly(true);
    
    consoleLayout->addWidget(consoleOutput);
    layout->addWidget(consoleGroup);
    
    layout->addStretch();
}

QGroupBox* MainWindow::setupTelemetryPanel()
{
    QGroupBox *telemetryGroup = new QGroupBox("📡 Telemetry Data");
    QGridLayout *layout = new QGridLayout(telemetryGroup);
    
    // Battery
    layout->addWidget(new QLabel("🔋 Battery:"), 0, 0);
    batteryLCD = new QLCDNumber(3);
    batteryLCD->setStyleSheet("QLCDNumber { background-color: #1A1A1A; color: #4CAF50; }");
    batteryLCD->display(100);
    layout->addWidget(batteryLCD, 0, 1);
    layout->addWidget(new QLabel("%"), 0, 2);
    
    // GPS
    layout->addWidget(new QLabel("🛰️ GPS Sats:"), 1, 0);
    gpsLabel = new QLabel("0");
    gpsLabel->setStyleSheet("QLabel { font-weight: bold; }");
    layout->addWidget(gpsLabel, 1, 1);
    
    // Altitude
    layout->addWidget(new QLabel("📏 Altitude:"), 2, 0);
    altitudeLabel = new QLabel("0.0 m");
    altitudeLabel->setStyleSheet("QLabel { font-weight: bold; }");
    layout->addWidget(altitudeLabel, 2, 1);
    
    // Speed
    layout->addWidget(new QLabel("💨 Speed:"), 3, 0);
    speedLabel = new QLabel("0.0 m/s");
    speedLabel->setStyleSheet("QLabel { font-weight: bold; }");
    layout->addWidget(speedLabel, 3, 1);
    
    // Heading
    layout->addWidget(new QLabel("🧭 Heading:"), 4, 0);
    headingLabel = new QLabel("0°");
    headingLabel->setStyleSheet("QLabel { font-weight: bold; }");
    layout->addWidget(headingLabel, 4, 1);
    
    // Signal strength
    layout->addWidget(new QLabel("📶 Signal:"), 5, 0);
    signalStrengthBar = new QProgressBar;
    signalStrengthBar->setRange(0, 100);
    signalStrengthBar->setValue(0);
    signalStrengthBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
    layout->addWidget(signalStrengthBar, 5, 1, 1, 2);
    
    return telemetryGroup;
}

void MainWindow::setupMapTab()
{
    mapWidget = new MapWidget;
    
    // Connect map signals
    connect(mapWidget, &MapWidget::waypointAdded, this, &MainWindow::onWaypointAdded);
    connect(mapWidget, &MapWidget::waypointRemoved, this, &MainWindow::onWaypointRemoved);
    
    // Set home position
    mapWidget->setHomePosition(latitude, longitude);
}

void MainWindow::setupFlightDataTab()
{
    flightDataWidget = new FlightDataWidget;
}

void MainWindow::setupFlightPlanTab()
{
    missionPlanWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(missionPlanWidget);
    
    // Mission controls
    QHBoxLayout *controlsLayout = new QHBoxLayout;
    
    addWaypointBtn = new QPushButton("➕ Add Waypoint");
    addWaypointBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");
    
    removeWaypointBtn = new QPushButton("➖ Remove Selected");
    removeWaypointBtn->setStyleSheet("QPushButton { background-color: #FF5722; color: white; }");
    
    clearWaypointsBtn = new QPushButton("🗑️ Clear All");
    clearWaypointsBtn->setStyleSheet("QPushButton { background-color: #9E9E9E; color: white; }");
    
    uploadMissionBtn = new QPushButton("⬆️ Upload Mission");
    uploadMissionBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");
    
    downloadMissionBtn = new QPushButton("⬇️ Download Mission");
    downloadMissionBtn->setStyleSheet("QPushButton { background-color: #FF9800; color: white; }");
    
    startMissionBtn = new QPushButton("▶️ Start Mission");
    startMissionBtn->setStyleSheet("QPushButton { background-color: #9C27B0; color: white; }");
    
    controlsLayout->addWidget(addWaypointBtn);
    controlsLayout->addWidget(removeWaypointBtn);
    controlsLayout->addWidget(clearWaypointsBtn);
    controlsLayout->addStretch();
    controlsLayout->addWidget(uploadMissionBtn);
    controlsLayout->addWidget(downloadMissionBtn);
    controlsLayout->addWidget(startMissionBtn);
    
    layout->addLayout(controlsLayout);
    
    // Waypoint table
    waypointTable = new QTableWidget;
    waypointTable->setColumnCount(6);
    waypointTable->setHorizontalHeaderLabels({"#", "Latitude", "Longitude", "Altitude", "Speed", "Action"});
    waypointTable->horizontalHeader()->setStretchLastSection(true);
    waypointTable->setAlternatingRowColors(true);
    waypointTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    layout->addWidget(waypointTable);
    
    // Connect signals
    connect(addWaypointBtn, &QPushButton::clicked, this, &MainWindow::showFlightPlanDialog);
    connect(clearWaypointsBtn, &QPushButton::clicked, [this]() {
        waypointTable->setRowCount(0);
        if (mapWidget) mapWidget->clearWaypoints();
        logMessage("All waypoints cleared");
    });
}

void MainWindow::setupSettingsTab()
{
    settingsWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(settingsWidget);
    
    settingsTabs = new QTabWidget;
    
    // Connection Settings Tab
    connectionSettingsTab = new QWidget;
    QFormLayout *connLayout = new QFormLayout(connectionSettingsTab);
    
    // Add connection settings widgets here
    QComboBox *portCombo = new QComboBox;
    portCombo->addItems({"COM1", "COM2", "COM3", "UDP:14550", "TCP:5760"});
    connLayout->addRow("Connection Type:", portCombo);
    
    QSpinBox *baudRate = new QSpinBox;
    baudRate->setRange(9600, 921600);
    baudRate->setValue(57600);
    connLayout->addRow("Baud Rate:", baudRate);
    
    // Flight Settings Tab
    flightSettingsTab = new QWidget;
    QFormLayout *flightLayout = new QFormLayout(flightSettingsTab);
    
    QDoubleSpinBox *maxAltitude = new QDoubleSpinBox;
    maxAltitude->setRange(0, 1000);
    maxAltitude->setValue(120);
    maxAltitude->setSuffix(" m");
    flightLayout->addRow("Max Altitude:", maxAltitude);
    
    QDoubleSpinBox *maxSpeed = new QDoubleSpinBox;
    maxSpeed->setRange(0, 50);
    maxSpeed->setValue(15);
    maxSpeed->setSuffix(" m/s");
    flightLayout->addRow("Max Speed:", maxSpeed);
    
    // Display Settings Tab
    displaySettingsTab = new QWidget;
    QFormLayout *displayLayout = new QFormLayout(displaySettingsTab);
    
    QCheckBox *darkMode = new QCheckBox;
    darkMode->setChecked(true);
    displayLayout->addRow("Dark Mode:", darkMode);
    
    QCheckBox *showGrid = new QCheckBox;
    showGrid->setChecked(true);
    displayLayout->addRow("Show Map Grid:", showGrid);
    
    // Add tabs
    settingsTabs->addTab(connectionSettingsTab, "Connection");
    settingsTabs->addTab(flightSettingsTab, "Flight");
    settingsTabs->addTab(displaySettingsTab, "Display");
    
    layout->addWidget(settingsTabs);
    layout->addStretch();
}

void MainWindow::setupArtificialHorizon()
{
    artificialHorizonView = new QGraphicsView;
    artificialHorizonView->setMinimumSize(250, 250);
    artificialHorizonView->setMaximumSize(300, 300);
    
    horizonScene = new QGraphicsScene(-125, -125, 250, 250);
    artificialHorizonView->setScene(horizonScene);
    artificialHorizonView->setRenderHint(QPainter::Antialiasing);
    
    // Create horizon elements
    horizonCircle = horizonScene->addEllipse(-100, -100, 200, 200, QPen(Qt::white, 2), QBrush(QColor(135, 206, 235))); // Sky blue
    
    // Ground
    QGraphicsRectItem *ground = horizonScene->addRect(-100, 0, 200, 100, QPen(Qt::white, 1), QBrush(QColor(139, 69, 19))); // Brown
    
    // Horizon line
    horizonScene->addLine(-100, 0, 100, 0, QPen(Qt::white, 3));
    
    // Aircraft symbol
    QGraphicsLineItem *aircraftH = horizonScene->addLine(-20, 0, 20, 0, QPen(Qt::yellow, 4));
    QGraphicsLineItem *aircraftV = horizonScene->addLine(0, -5, 0, 5, QPen(Qt::yellow, 4));
    
    // Attitude text
    rollText = horizonScene->addText("Roll: 0°", QFont("Arial", 10));
    rollText->setDefaultTextColor(Qt::white);
    rollText->setPos(-120, -140);
    
    pitchText = horizonScene->addText("Pitch: 0°", QFont("Arial", 10));
    pitchText->setDefaultTextColor(Qt::white);
    pitchText->setPos(-120, -155);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Mission", this, &MainWindow::showFlightPlanDialog);
    fileMenu->addAction("&Open Mission", this, [this](){ 
        logMessage("Open mission dialog would be implemented here"); 
    });
    fileMenu->addAction("&Save Mission", this, [this](){ 
        logMessage("Save mission dialog would be implemented here"); 
    });
    fileMenu->addSeparator();
    fileMenu->addAction("&Import Waypoints", this, [this](){ 
        logMessage("Import waypoints functionality would be implemented here"); 
    });
    fileMenu->addAction("&Export Waypoints", this, [this](){ 
        logMessage("Export waypoints functionality would be implemented here"); 
    });
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close);
    
    // Vehicle menu
    QMenu *vehicleMenu = menuBar->addMenu("&Vehicle");
    vehicleMenu->addAction("&Connect", this, &MainWindow::connectToUAV);
    vehicleMenu->addAction("&Disconnect", this, &MainWindow::disconnectFromUAV);
    vehicleMenu->addSeparator();
    vehicleMenu->addAction("&Calibrate Sensors", this, &MainWindow::showCalibrationDialog);
    vehicleMenu->addAction("&Parameters", this, &MainWindow::showParameterDialog);
    vehicleMenu->addAction("&Firmware Update", this, [this](){ 
        logMessage("Firmware update dialog would be implemented here"); 
    });
    
    // Mission menu
    QMenu *missionMenu = menuBar->addMenu("&Mission");
    missionMenu->addAction("&Plan Mission", this, [this](){ 
        mainTabs->setCurrentIndex(2); // Switch to mission plan tab
    });
    missionMenu->addAction("&Upload Mission", this, [this](){ 
        logMessage("Upload mission to vehicle"); 
    });
    missionMenu->addAction("&Download Mission", this, [this](){ 
        logMessage("Download mission from vehicle"); 
    });
    missionMenu->addAction("&Start Mission", this, [this](){ 
        logMessage("Mission started"); 
        flightModeCombo->setCurrentText("Auto Mission");
    });
    missionMenu->addAction("&Pause Mission", this, [this](){ 
        logMessage("Mission paused"); 
    });
    missionMenu->addAction("&Resume Mission", this, [this](){ 
        logMessage("Mission resumed"); 
    });
    
    // Tools menu
    QMenu *toolsMenu = menuBar->addMenu("&Tools");
    toolsMenu->addAction("&Flight Data Analysis", this, [this](){ 
        mainTabs->setCurrentIndex(1); // Switch to flight data tab
    });
    toolsMenu->addAction("&Log Browser", this, &MainWindow::showLogBrowser);
    toolsMenu->addAction("&MAVLink Inspector", this, [this](){ 
        logMessage("MAVLink inspector would be implemented here"); 
    });
    toolsMenu->addAction("&Console", this, [this](){ 
        logMessage("Console window would be implemented here"); 
    });
    
    // View menu
    QMenu *viewMenu = menuBar->addMenu("&View");
    viewMenu->addAction("&Map View", this, [this](){ 
        mainTabs->setCurrentIndex(0); 
    });
    viewMenu->addAction("&Flight Data", this, [this](){ 
        mainTabs->setCurrentIndex(1); 
    });
    viewMenu->addAction("&Mission Planner", this, [this](){ 
        mainTabs->setCurrentIndex(2); 
    });
    viewMenu->addAction("&Settings", this, [this](){ 
        mainTabs->setCurrentIndex(3); 
    });
    viewMenu->addSeparator();
    viewMenu->addAction("&Full Screen", this, [this](){ 
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });
    
    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&User Manual", this, [this](){ 
        logMessage("User manual would be opened here"); 
    });
    helpMenu->addAction("&Keyboard Shortcuts", this, [this](){
        QString shortcuts = "Keyboard Shortcuts:\n\n"
                          "Ctrl+N - New Mission\n"
                          "Ctrl+O - Open Mission\n"
                          "Ctrl+S - Save Mission\n"
                          "F11 - Toggle Full Screen\n"
                          "Ctrl+1 - Map View\n"
                          "Ctrl+2 - Flight Data\n"
                          "Ctrl+3 - Mission Planner\n"
                          "Ctrl+4 - Settings\n"
                          "Space - Emergency Stop\n"
                          "A - Arm/Disarm\n"
                          "T - Take Off\n"
                          "L - Land\n"
                          "R - Return to Home";
        QMessageBox::information(this, "Keyboard Shortcuts", shortcuts);
    });
    helpMenu->addAction("&About Qt", qApp, &QApplication::aboutQt);
    helpMenu->addAction("&About KL-UAS GCS", this, [this](){
        QMessageBox::about(this, "About KL-UAS GCS", 
            "KL-UAS Ground Control Station\n"
            "Version 2.0\n\n"
            "A comprehensive ground control station for UAV operations.\n"
            "Built with Qt6 and designed for professional use.\n\n"
            "Features:\n"
            "• Real-time telemetry monitoring\n"
            "• Mission planning and execution\n"
            "• Flight data analysis\n"
            "• Interactive map interface\n"
            "• Artificial horizon display\n"
            "• Comprehensive logging system");
    });
}

void MainWindow::setupStatusBar()
{
    QStatusBar *statusBar = this->statusBar();
    statusBar->showMessage("Ready");
    
    // Add permanent widgets to status bar
    batteryLabel = new QLabel("Battery: 100%");
    batteryLabel->setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; }");
    statusBar->addPermanentWidget(batteryLabel);
}

void MainWindow::updateTelemetry()
{
    if (isConnected) {
        simulateTelemetryData();
        
        // Update displays
        batteryLCD->display(batteryPercentage);
        gpsLabel->setText(QString::number(satelliteCount));
        altitudeLabel->setText(QString("%1 m").arg(altitude, 0, 'f', 1));
        speedLabel->setText(QString("%1 m/s").arg(speed, 0, 'f', 1));
        headingLabel->setText(QString("%1°").arg(heading, 0, 'f', 0));
        signalStrengthBar->setValue(signalStrength);
        
        // Update status bar
        batteryLabel->setText(QString("Battery: %1%").arg(batteryPercentage));
        if (batteryPercentage < 20) {
            batteryLabel->setStyleSheet("QLabel { color: #F44336; font-weight: bold; }");
        } else if (batteryPercentage < 50) {
            batteryLabel->setStyleSheet("QLabel { color: #FF9800; font-weight: bold; }");
        } else {
            batteryLabel->setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; }");
        }
        
        // Update map widget
        if (mapWidget) {
            mapWidget->updateDronePosition(latitude, longitude, heading);
        }
        
        // Update flight data widget
        if (flightDataWidget) {
            flightDataWidget->updateFlightData(altitude, speed, roll, pitch, yaw, 
                                             batteryPercentage, satelliteCount, signalStrength);
            flightDataWidget->updateGPSData(latitude, longitude, altitude);
        }
    }
}

void MainWindow::onWaypointAdded(double lat, double lon)
{
    int row = waypointTable->rowCount();
    waypointTable->insertRow(row);
    
    waypointTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    waypointTable->setItem(row, 1, new QTableWidgetItem(QString::number(lat, 'f', 6)));
    waypointTable->setItem(row, 2, new QTableWidgetItem(QString::number(lon, 'f', 6)));
    waypointTable->setItem(row, 3, new QTableWidgetItem("50")); // Default altitude
    waypointTable->setItem(row, 4, new QTableWidgetItem("5")); // Default speed
    waypointTable->setItem(row, 5, new QTableWidgetItem("Waypoint"));
    
    logMessage(QString("Waypoint %1 added at %2, %3").arg(row + 1).arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6));
}

void MainWindow::onWaypointRemoved(int index)
{
    if (index >= 0 && index < waypointTable->rowCount()) {
        waypointTable->removeRow(index);
        updateWaypointTable();
        logMessage(QString("Waypoint %1 removed").arg(index + 1));
    }
}

void MainWindow::updateWaypointTable()
{
    // Renumber waypoints
    for (int i = 0; i < waypointTable->rowCount(); ++i) {
        waypointTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
    }
}

void MainWindow::showFlightPlanDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Add Waypoint");
    dialog.setModal(true);
    
    QFormLayout *layout = new QFormLayout(&dialog);
    
    QDoubleSpinBox *latSpinBox = new QDoubleSpinBox;
    latSpinBox->setRange(-90, 90);
    latSpinBox->setDecimals(6);
    latSpinBox->setValue(latitude);
    layout->addRow("Latitude:", latSpinBox);
    
    QDoubleSpinBox *lonSpinBox = new QDoubleSpinBox;
    lonSpinBox->setRange(-180, 180);
    lonSpinBox->setDecimals(6);
    lonSpinBox->setValue(longitude);
    layout->addRow("Longitude:", lonSpinBox);
    
    QSpinBox *altSpinBox = new QSpinBox;
    altSpinBox->setRange(0, 1000);
    altSpinBox->setValue(50);
    altSpinBox->setSuffix(" m");
    layout->addRow("Altitude:", altSpinBox);
    
    QSpinBox *speedSpinBox = new QSpinBox;
    speedSpinBox->setRange(1, 20);
    speedSpinBox->setValue(5);
    speedSpinBox->setSuffix(" m/s");
    layout->addRow("Speed:", speedSpinBox);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if (dialog.exec() == QDialog::Accepted) {
        double lat = latSpinBox->value();
        double lon = lonSpinBox->value();
        
        if (mapWidget) {
            mapWidget->addWaypoint(lat, lon);
        }
    }
}

void MainWindow::showCalibrationDialog()
{
    QMessageBox::information(this, "Calibration", 
        "Calibration dialog would be implemented here.\n"
        "This would include:\n"
        "• Accelerometer calibration\n"
        "• Magnetometer calibration\n"
        "• ESC calibration\n"
        "• Radio calibration");
}

void MainWindow::showParameterDialog()
{
    QMessageBox::information(this, "Parameters", 
        "Parameter editor would be implemented here.\n"
        "This would allow editing of:\n"
        "• Flight controller parameters\n"
        "• PID tuning values\n"
        "• Safety settings\n"
        "• Sensor configurations");
}

void MainWindow::showLogBrowser()
{
    QMessageBox::information(this, "Log Browser", 
        "Log browser would be implemented here.\n"
        "This would allow:\n"
        "• Viewing flight logs\n"
        "• Analyzing telemetry data\n"
        "• Exporting log data\n"
        "• Log file management");
}

void MainWindow::simulateTelemetryData()
{
    // Simulate realistic telemetry data
    static double time = 0;
    time += 0.1;
    
    if (isArmed) {
        // Simulate flight
        altitude += (QRandomGenerator::global()->bounded(0.2) - 0.1);
        altitude = qMax(0.0, altitude);
        
        speed = 5.0 + 3.0 * sin(time * 0.5);
        heading += (QRandomGenerator::global()->bounded(2.0) - 1.0);
        if (heading < 0) heading += 360;
        if (heading >= 360) heading -= 360;
        
        roll = 15.0 * sin(time * 0.8);
        pitch = 10.0 * cos(time * 0.6);
        
        batteryPercentage = qMax(0, batteryPercentage - (QRandomGenerator::global()->bounded(2) ? 0 : 1));
    } else {
        speed = 0;
        roll = roll * 0.95; // Gradually return to level
        pitch = pitch * 0.95;
    }
    
    satelliteCount = isConnected ? (8 + QRandomGenerator::global()->bounded(5)) : 0;
    signalStrength = isConnected ? (70 + QRandomGenerator::global()->bounded(30)) : 0;
}

void MainWindow::updateArtificialHorizon()
{
    if (!horizonScene) return;
    
    // Clear previous horizon elements (except static ones)
    horizonScene->clear();
    
    // Recreate horizon with current attitude
    double rollRad = roll * M_PI / 180.0;
    double pitchOffset = pitch * 2; // Scale pitch for visibility
    
    // Sky
    QGraphicsEllipseItem *sky = horizonScene->addEllipse(-100, -100, 200, 200, 
        QPen(Qt::white, 2), QBrush(QColor(135, 206, 235)));
    
    // Ground (rotated and offset by pitch)
    QPolygonF groundPoly;
    groundPoly << QPointF(-100, pitchOffset) << QPointF(100, pitchOffset) 
               << QPointF(100, 100) << QPointF(-100, 100);
    
    QGraphicsPolygonItem *ground = horizonScene->addPolygon(groundPoly, 
        QPen(Qt::white, 1), QBrush(QColor(139, 69, 19)));
    ground->setTransformOriginPoint(0, 0);
    ground->setRotation(roll);
    
    // Horizon line
    QGraphicsLineItem *horizonLine = horizonScene->addLine(-100, pitchOffset, 100, pitchOffset, QPen(Qt::white, 3));
    horizonLine->setTransformOriginPoint(0, 0);
    horizonLine->setRotation(roll);
    
    // Aircraft symbol (fixed)
    horizonScene->addLine(-20, 0, 20, 0, QPen(Qt::yellow, 4));
    horizonScene->addLine(0, -5, 0, 5, QPen(Qt::yellow, 4));
    
    // Roll indicator marks
    for (int i = -60; i <= 60; i += 30) {
        if (i == 0) continue;
        double rad = i * M_PI / 180.0;
        double x1 = 90 * sin(rad);
        double y1 = -90 * cos(rad);
        double x2 = 95 * sin(rad);
        double y2 = -95 * cos(rad);
        horizonScene->addLine(x1, y1, x2, y2, QPen(Qt::white, 2));
    }
    
    // Attitude text
    rollText = horizonScene->addText(QString("Roll: %1°").arg(roll, 0, 'f', 1), QFont("Arial", 10));
    rollText->setDefaultTextColor(Qt::white);
    rollText->setPos(-120, -140);
    
    pitchText = horizonScene->addText(QString("Pitch: %1°").arg(pitch, 0, 'f', 1), QFont("Arial", 10));
    pitchText->setDefaultTextColor(Qt::white);
    pitchText->setPos(-120, -155);
}

void MainWindow::connectToUAV()
{
    if (!isConnected) {
        isConnected = true;
        connectBtn->setText("Disconnect");
        connectBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
        connectionStatusLabel->setText("Connected");
        connectionStatusLabel->setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; }");
        
        // Enable controls
        armBtn->setEnabled(true);
        flightModeCombo->setEnabled(true);
        emergencyBtn->setEnabled(true);
        
        logMessage("Connected to UAV");
        statusBar()->showMessage("Connected to UAV");
        
        disconnect(connectBtn, &QPushButton::clicked, this, &MainWindow::connectToUAV);
        connect(connectBtn, &QPushButton::clicked, this, &MainWindow::disconnectFromUAV);
    }
}

void MainWindow::disconnectFromUAV()
{
    if (isConnected) {
        isConnected = false;
        isArmed = false;
        
        connectBtn->setText("Connect");
        connectBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
        connectionStatusLabel->setText("Disconnected");
        connectionStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        
        // Disable controls
        armBtn->setEnabled(false);
        armBtn->setText("ARM");
        armBtn->setStyleSheet("QPushButton { background-color: #ff9800; color: white; font-weight: bold; padding: 8px; }");
        takeOffBtn->setEnabled(false);
        landBtn->setEnabled(false);
        rthBtn->setEnabled(false);
        flightModeCombo->setEnabled(false);
        emergencyBtn->setEnabled(false);
        
        // Reset telemetry
        batteryLCD->display(0);
        gpsLabel->setText("0");
        altitudeLabel->setText("0.0 m");
        speedLabel->setText("0.0 m/s");
        headingLabel->setText("0°");
        signalStrengthBar->setValue(0);
        
        logMessage("Disconnected from UAV");
        statusBar()->showMessage("Disconnected");
        
        disconnect(connectBtn, &QPushButton::clicked, this, &MainWindow::disconnectFromUAV);
        connect(connectBtn, &QPushButton::clicked, this, &MainWindow::connectToUAV);
    }
}

void MainWindow::armDisarm()
{
    if (!isConnected) return;
    
    if (!isArmed) {
        isArmed = true;
        armBtn->setText("DISARM");
        armBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
        
        takeOffBtn->setEnabled(true);
        landBtn->setEnabled(true);
        rthBtn->setEnabled(true);
        
        logMessage("UAV ARMED - Motors spinning");
        statusBar()->showMessage("ARMED");
    } else {
        isArmed = false;
        armBtn->setText("ARM");
        armBtn->setStyleSheet("QPushButton { background-color: #ff9800; color: white; font-weight: bold; padding: 8px; }");
        
        takeOffBtn->setEnabled(false);
        landBtn->setEnabled(false);
        rthBtn->setEnabled(false);
        
        altitude = 0.0;
        speed = 0.0;
        
        logMessage("UAV DISARMED");
        statusBar()->showMessage("DISARMED");
    }
}

void MainWindow::takeOff()
{
    if (isArmed) {
        logMessage("Take-off command sent");
        statusBar()->showMessage("Taking off...");
        altitude = 10.0; // Simulate takeoff to 10m
    }
}

void MainWindow::land()
{
    if (isArmed) {
        logMessage("Landing command sent");
        statusBar()->showMessage("Landing...");
    }
}

void MainWindow::returnToHome()
{
    if (isArmed) {
        logMessage("Return to Home activated");
        statusBar()->showMessage("Returning to home...");
        flightModeCombo->setCurrentText("RTH");
    }
}

void MainWindow::emergencyStop()
{
    if (isConnected) {
        QMessageBox::StandardButton reply = QMessageBox::warning(this, "Emergency Stop", 
            "Are you sure you want to perform an emergency stop?\nThis will immediately cut power to motors!",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            isArmed = false;
            armBtn->setText("ARM");
            armBtn->setStyleSheet("QPushButton { background-color: #ff9800; color: white; font-weight: bold; padding: 8px; }");
            
            takeOffBtn->setEnabled(false);
            landBtn->setEnabled(false);
            rthBtn->setEnabled(false);
            
            altitude = 0.0;
            speed = 0.0;
            
            logMessage("EMERGENCY STOP ACTIVATED!");
            statusBar()->showMessage("EMERGENCY STOP");
        }
    }
}

void MainWindow::onModeChanged()
{
    QString mode = flightModeCombo->currentText();
    logMessage(QString("Flight mode changed to: %1").arg(mode));
    statusBar()->showMessage(QString("Mode: %1").arg(mode));
}

void MainWindow::updateConnectionStatus()
{
    // Implementation handled in connect/disconnect functions
}

void MainWindow::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, message);
    consoleOutput->append(logEntry);
    
    // Auto-scroll to bottom
    QTextCursor cursor = consoleOutput->textCursor();
    cursor.movePosition(QTextCursor::End);
    consoleOutput->setTextCursor(cursor);
    
    // Limit console to 100 lines
    if (consoleOutput->document()->lineCount() > 100) {
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 1);
        cursor.removeSelectedText();
    }
}
