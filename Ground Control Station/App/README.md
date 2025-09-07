# KL-UAS Ground Control Station v2.0

A comprehensive Ground Control Station (GCS) application built with Qt6 for professional UAV/Drone control and monitoring.

## 🚀 Features Overview

### 🎮 Flight Controls
- **Connection Management**: Connect/Disconnect to UAV with status monitoring
- **Arming/Disarming**: Safe motor control with visual feedback
- **Flight Commands**: Takeoff, Land, Return to Home with confirmation dialogs
- **Emergency Stop**: Immediate motor cutoff with safety confirmation
- **Flight Modes**: Manual, Stabilize, Altitude Hold, Position Hold, RTH, Auto Mission

### �️ Interactive Map Interface
- **Real-time Drone Tracking**: Live position updates with heading indicator
- **Mission Planning**: Click-to-add waypoints with visual connections
- **Flight Path Visualization**: Historical flight path with customizable display
- **Home Position**: Clearly marked takeoff/landing location
- **Context Menu**: Right-click for quick actions (zoom, waypoint management)
- **Zoom Controls**: Mouse wheel zoom with reset functionality

### 📊 Advanced Flight Data Analysis
- **Real-time Telemetry**: Battery, GPS, altitude, speed, heading monitoring
- **Attitude Display**: Roll, pitch, yaw with visual progress bars
- **System Health**: Battery voltage, current, power, temperature monitoring
- **Live Charts**: Altitude, speed, and battery trends over time
- **Data Logging**: Exportable CSV flight data with timestamps
- **GPS Precision**: HDOP, fix type, and satellite count display

### 🛩️ Professional Artificial Horizon
- **Real-time Attitude**: Dynamic roll and pitch visualization
- **Aircraft Symbol**: Fixed reference point with clear indicators
- **Horizon Line**: Smooth orientation updates at 50fps
- **Attitude Values**: Numerical roll/pitch displays
- **Roll Indicators**: Visual angle reference marks

### ✈️ Mission Planning System
- **Waypoint Management**: Add, remove, and edit mission waypoints
- **Interactive Table**: Editable waypoint parameters (lat, lon, alt, speed)
- **Mission Upload/Download**: Sync missions with flight controller
- **Visual Planning**: Integrated with map for intuitive planning
- **Mission Execution**: Start, pause, resume mission controls

### ⚙️ Comprehensive Settings
- **Connection Settings**: Port selection, baud rate configuration
- **Flight Parameters**: Max altitude, speed limits, safety settings
- **Display Options**: Dark mode, grid display, UI customization
- **Calibration Tools**: Sensor calibration workflows

### �️ Professional User Interface
- **Tabbed Layout**: Organized workflow with Map, Flight Data, Mission Plan, Settings
- **Dark Theme**: Eye-friendly professional aviation interface
- **Responsive Design**: Scalable interface for different screen sizes
- **Status Integration**: Real-time status bar with critical information
- **Menu System**: Comprehensive menu structure with keyboard shortcuts

## 📋 Requirements

- **Qt6**: Core, Widgets, Charts modules
- **CMake 3.16+**
- **C++17 compiler**
- **Visual Studio 2019+ (Windows) / GCC 9+ (Linux) / Clang 10+ (macOS)**

## 🔧 Building the Application

### Windows

#### Option 1: Using PowerShell (Recommended)
```powershell
.\build.ps1
```

#### Option 2: Using Command Prompt
```cmd
build.bat
```

#### Option 3: Manual Build
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux/macOS
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## 📁 Project Structure

```
App/
├── main.cpp                    # Application entry point
├── mainwindow.h/.cpp/.ui       # Main application window
├── mapwidget.h/.cpp           # Interactive map component
├── flightdatawidget.h/.cpp    # Flight data analysis panel
├── CMakeLists.txt             # Build configuration
├── build.bat                  # Windows build script
├── build.ps1                  # PowerShell build script
└── README.md                  # This documentation
```

## 🎯 Usage Guide

### Initial Setup
1. **Launch Application**: Run `KL_UAS_GCS.exe`
2. **Configure Connection**: Go to Settings → Connection tab
3. **Set Flight Parameters**: Configure in Settings → Flight tab

### Flight Operations
1. **Connect to UAV**: Click "Connect to UAV" button
2. **Monitor Telemetry**: View real-time data in control panel
3. **Plan Mission**: Switch to Mission Plan tab, add waypoints
4. **Execute Flight**:
   - ARM the vehicle
   - Use flight control buttons
   - Monitor on Map and Flight Data tabs

### Data Analysis
1. **View Live Charts**: Switch to Flight Data tab
2. **Start Recording**: Click "Start Recording" for data logging
3. **Export Data**: Use "Export Data" for CSV export
4. **Analyze Trends**: Monitor altitude, speed, battery charts

## 🛡️ Safety Features

- **Emergency Stop**: Requires confirmation dialog
- **State Management**: Proper enable/disable of controls based on connection status
- **Visual Feedback**: Color-coded status indicators throughout interface
- **Connection Monitoring**: Clear connection state display
- **Parameter Validation**: Input validation for mission planning
- **Altitude Limits**: Configurable maximum altitude enforcement

## 📊 Simulated Data Features

The application includes realistic telemetry simulation for development and demonstration:
- **Progressive Battery Drain**: Realistic discharge during flight operations
- **Dynamic Attitude Changes**: Smooth roll/pitch variations during flight
- **GPS Satellite Variation**: Realistic satellite count fluctuations
- **Signal Strength Modeling**: Connection quality simulation
- **Environmental Factors**: Temperature, vibration simulation

## 🎹 Keyboard Shortcuts

- **Ctrl+N**: New Mission
- **Ctrl+O**: Open Mission
- **Ctrl+S**: Save Mission
- **F11**: Toggle Full Screen
- **Ctrl+1**: Map View
- **Ctrl+2**: Flight Data View
- **Ctrl+3**: Mission Planner
- **Ctrl+4**: Settings
- **Space**: Emergency Stop
- **A**: Arm/Disarm
- **T**: Take Off
- **L**: Land
- **R**: Return to Home

## 🔮 Future Enhancements

- [ ] **Real MAVLink Integration**: Support for MAVLink protocol communication
- [ ] **Advanced Map Services**: Integration with Google Maps/OpenStreetMap
- [ ] **Video Stream Support**: Live camera feed integration
- [ ] **Multi-Vehicle Support**: Manage multiple UAVs simultaneously
- [ ] **Plugin Architecture**: Extensible plugin system
- [ ] **Advanced Analytics**: Machine learning flight analysis
- [ ] **Cloud Integration**: Cloud-based mission storage and sharing
- [ ] **Mobile Companion**: iOS/Android companion apps
- [ ] **3D Visualization**: 3D flight path and terrain visualization
- [ ] **Weather Integration**: Real-time weather data overlay

## 🏗️ Technical Architecture

### Core Components
- **Model-View Separation**: Clean separation between UI and logic
- **Timer-Based Updates**: Efficient 100ms telemetry refresh cycle
- **Signal-Slot Connections**: Qt's robust event handling system
- **Graphics Framework**: Custom QGraphicsView implementations
- **Responsive Design**: Adaptive layouts for various screen sizes

### Performance Optimizations
- **Efficient Rendering**: Hardware-accelerated graphics where available
- **Memory Management**: Smart pointer usage and proper cleanup
- **Update Throttling**: Appropriate refresh rates for different components
- **Data Structures**: Optimized containers for telemetry data

### Styling & Theming
- **Professional Dark Theme**: Reduced eye strain for long operations
- **Color-Coded Status**: Intuitive visual feedback system
- **Consistent Typography**: Professional aviation-grade fonts
- **Scalable Icons**: Vector-based interface elements

## 🤝 Contributing

1. **Code Style**: Follow Qt coding conventions and C++17 standards
2. **UI Consistency**: Maintain dark theme and professional appearance
3. **Error Handling**: Add appropriate error handling and user feedback
4. **Documentation**: Update documentation for new features
5. **Testing**: Test on multiple platforms before submitting

## 📄 License

This project is part of the KL-UAS System. See main project LICENSE file for details.

## 📞 Support & Contact

For questions, bug reports, or feature requests:
- Create an issue in the project repository
- Refer to the main KL-UAS project documentation
- Check the built-in help system (Help → User Manual)

---

**KL-UAS Ground Control Station** - Professional UAV Operations Made Simple
