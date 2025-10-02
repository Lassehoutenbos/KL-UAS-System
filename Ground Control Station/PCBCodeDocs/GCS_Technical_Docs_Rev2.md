# Ground Control Station - Bottom Panel Components Documentation

This document provides a comprehensive overview of all individual components and subsystems in the bottom panel of the Ground Control Station (GCS).

## System Overview

The bottom panel serves as the main control interface and processing hub for the Ground Control Station. It contains the primary microcontroller, display, switches, status indicators, and power management systems needed for drone mission control.

## Core Processing Components

### Main Controller (STM32F411CE)

- **Type**: 32-bit ARM Cortex-M4 microcontroller (BlackPill development board)
- **Clock Speed**: Up to 100 MHz
- **Flash Memory**: 512 KB
- **RAM**: 128 KB
- **Connectivity**: USB 2.0 Full Speed, SPI, I²C, UART interfaces
- **Power**: 3.3V operation with integrated voltage regulation
- **Purpose**: Central processing unit for all panel operations, runs FreeRTOS for reliable multitasking

### I/O Expansion (MCP23017)

- **Type**: 16-bit I/O expander with I²C interface
- **Communication**: I²C bus (SCL: PB7, SDA: PB8)
- **Functionality**: Provides additional GPIO pins for switches and LEDs
- **Pin Allocation**:
  - **Port A (A0-A7)**: LED control (LED2-LED4), key input, switch inputs (SW3_3, SW3_4)
  - **Port B (B0-B7)**: Switch matrix (SW1_1 to SW3_2)
- **Voltage**: 3.3V compatible
- **Purpose**: Expands the limited GPIO pins of the STM32 for handling multiple switches and indicators

### PWM Controller (PCA9685)

- **Type**: 16-channel, 12-bit PWM driver
- **Communication**: I²C interface
- **Resolution**: 12-bit (4096 steps)
- **Frequency Range**: 24 Hz to 1526 Hz
- **Channels Used**:
  - **Channels 0-5**: RGB LED control (LED5_R, LED5_G, LED5_B, LED6_R, LED6_G, LED6_B)
  - **Channels 6-7**: Fan PWM control (FAN1_PWM_CH, FAN2_PWM_CH)
  - **Channels 8-15**: Available for expansion
- **Purpose**: Provides precise PWM control for RGB LEDs and cooling fans

## Display System

### TFT Display (ST7735)

- **Size**: 1.44 inches diagonal
- **Resolution**: 128×128 pixels
- **Color Depth**: 16-bit color (65,536 colors)
- **Interface**: SPI (SCK: PB13, MISO: PB14, MOSI: PB15)
- **Control Pins**:
  - **CS (Chip Select)**: PA9
  - **DC (Data/Command)**: PA10
  - **RST (Reset)**: PA8
- **Rotation**: Configured for 270° rotation (landscape orientation)
- **Features**:
  - **Power Status Display**: Shows battery and external power voltages
  - **Warning Screens**: Red warning overlay, yellow battery warning, lock screen
  - **Animated Interface**: Mechanical-style switch arm animation
  - **Real-time Updates**: Voltage monitoring with 1-second refresh rate

## Input Controls

### Switch Matrix

The panel features 10 switches (SW0-SW9) connected through the MCP23017 I/O expander:

- **SW0-SW2**: Connected to MCP23017 pins 8-10
- **SW3-SW7**: Connected to MCP23017 pins 11-15 (with integrated LED feedback)
- **SW8-SW9**: Connected to MCP23017 pins 6-7
- **Key Input**: Special key switch on MCP23017 pin 5

### Switch Features

- **Switch Types**: Mixed configuration of momentary and toggle switches
  - **SW0-SW2**: Momentary contact switches
  - **SW3**: Toggle switch with state memory
  - **SW4-SW9**: Momentary contact switches
- **LED Feedback**: Switches SW3-SW7 have integrated LED indicators
- **State Detection**: Hardware debouncing implemented in software
- **Safety Functions**:
  - **Lock State**: Global system lock/unlock capability
  - **Confirmation Required**: All switches must be in low position during unlock
  - **Payload Arming**: Dedicated payload arm controls (armPayload1, armPayload2)

## Status Indicators

### LED System Overview

The panel incorporates multiple LED types for comprehensive status indication:

### GPIO LEDs (Simple Status)

- **LED2-LED4**: Basic status indicators connected to MCP23017
- **Function**: Simple on/off status display
- **Control**: Direct GPIO control via I/O expander

### RGB LEDs (Advanced Status)

- **LED5-LED6**: Full RGB capability via PCA9685 PWM control
- **Color Range**: Full spectrum color mixing
- **Brightness**: 12-bit PWM resolution for smooth dimming
- **Applications**: System status, warning conditions, mode indication

### NeoPixel LED Strip

- **Type**: WS2812B addressable RGB LEDs
- **Count**: 38 individual LEDs
- **Control Pin**: PB3 (via level shifter)
- **Features**:
  - **Individual Control**: Each LED independently addressable
  - **Full Color**: 24-bit color depth per LED
  - **Switch Integration**: LEDs mapped to specific switches for visual feedback
  - **Effects**: Capable of animations, patterns, and breathing effects

### LED Mapping System

Each switch can be associated with both GPIO LEDs and NeoPixel strip segments:

```cpp
struct SwitchLedMapping {
    bool hasGpioLed;          // Has simple GPIO LED
    uint8_t gpioPin;          // GPIO pin number
    bool hasStripLed;         // Has NeoPixel strip LED(s)
    uint8_t stripStartIndex;  // First LED in strip
    uint8_t stripEndIndex;    // Last LED in strip
}
```

## Power Management

### Voltage Monitoring

- **Battery Input (BAT_VIN)**: PA0 (ADC0) - Internal battery voltage measurement
- **External Input (EXT_VIN)**: PA1 (ADC1) - External power source monitoring
- **Resolution**: 12-bit ADC (4096 levels)
- **Reference**: 3.3V system reference
- **Update Rate**: Continuous monitoring with display updates every 1 second

### Power Source Selection

- **Automatic Switching**: System can detect and switch between battery and external power
- **Visual Indication**: Animated display shows active power source
- **Voltage Display**: Real-time voltage readings for both sources
- **Threshold Monitoring**: Configurable voltage thresholds for warnings

### Power Distribution

- **3.3V Rail**: Primary system voltage for microcontroller and digital components
- **5V Rail**: Available for higher power components (via external power input)
- **USB Power**: Can power system during development/programming

## Thermal Management

### Temperature Sensing

- **Sensor Count**: 4 temperature sensors (SENS2-SENS5)
- **Pins**: PB0, PB1, PA6, PA7
- **Type**: Analog temperature sensors
- **Range**: Configurable based on sensor specifications
- **Update Rate**: 100ms polling interval

### Cooling System

- **Fan Count**: 2 PWM-controlled fans
- **Control**: PCA9685 channels 6 and 7
- **Speed Range**: 0-100% via 12-bit PWM
- **Control Logic**:
  - **Nominal Temperature**: Base temperature where fan starts increasing speed
  - **Maximum Temperature**: Temperature for full fan speed
  - **Sensor Mapping**: Each fan can be linked to multiple temperature sensors

### Thermal Control Algorithm

```cpp
struct FanSensorConfig {
    uint8_t sensorIndex;   // Linked temperature sensor (0-3)
    float nominalTemp;     // °C where fan starts increasing
    float maxTemp;         // °C for full speed
}
```

## Communication Interfaces

### USB HID Interface

- **Protocol**: USB Human Interface Device
- **Functions**: 
  - **Keyboard Emulation**: Can send keystrokes to host computer
  - **Custom HID**: Mission control commands and telemetry
- **Speed**: USB 2.0 Full Speed (12 Mbps)
- **Driver**: Standard HID drivers (no custom drivers required)

### I²C Bus

- **Speed**: Standard mode (100 kHz) and Fast mode (400 kHz) support
- **Devices**: MCP23017 I/O expander, PCA9685 PWM controller
- **Pull-ups**: External pull-up resistors required
- **Addresses**: Configurable device addresses to avoid conflicts

### SPI Bus

- **Speed**: Up to 40 MHz for display communication
- **Devices**: ST7735 TFT display
- **Mode**: SPI Mode 0 (CPOL=0, CPHA=0)
- **Data Width**: 8-bit transfers

## Software Architecture

### Real-Time Operating System (FreeRTOS)

- **Version**: STM32duino FreeRTOS 10.3.2
- **Task Management**: Multiple concurrent tasks for different subsystems
- **Key Tasks**:
  - **UI Task**: Display updates and switch handling (5ms cycle)
  - **Temperature Task**: Thermal monitoring and fan control (100ms cycle)
  - **Communication Task**: USB HID interface management

### Modular Library Structure

- **ScreenPowerSwitch**: Display management and power status
- **SwitchHandler**: Switch polling and event management  
- **TempSensors**: Temperature monitoring and fan control
- **Blinker**: LED effects and animations
- **Pins**: Centralized pin definitions and hardware abstraction

### Safety Features

- **Watchdog Timer**: System reset if firmware hangs
- **Switch Confirmation**: All switches must be confirmed before unlock
- **Lock States**: Multiple lock modes (warning, battery warning, full lock)
- **Error Handling**: Graceful handling of communication failures

## Physical Integration

### PCB Design

- **Custom PCB**: Designed specifically for the bottom panel housing
- **Connector Types**: Standard connectors for easy maintenance
- **Mounting**: Secure mounting points for vibration resistance
- **Access**: Debug ports accessible for development

### Enclosure Integration

- **Switch Mounting**: Switches mounted through panel face
- **Display Mounting**: Display visible through panel cutout
- **LED Integration**: LEDs positioned for optimal visibility
- **Cooling**: Ventilation design for thermal management

### Cable Management

- **Internal Routing**: Organized cable routing within enclosure
- **External Connections**: Standardized connectors for system integration
- **Service Access**: Easy access for maintenance and troubleshooting

## Development and Testing

### Programming Environment

- **Framework**: PlatformIO with Arduino compatibility
- **Compiler**: GCC ARM toolchain
- **Debugging**: SWD interface via ST-Link
- **Unit Testing**: Native environment testing for cross-platform development

### Testing Capabilities

- **Hardware-in-Loop**: Full system testing with actual hardware
- **Simulation Mode**: Software testing without hardware dependencies
- **Debug Modes**: Special compilation flags for testing individual subsystems
- **Automated Testing**: Unit tests for temperature control logic

This comprehensive system provides a robust and feature-rich control interface for drone operations, with redundant safety features and professional-grade hardware integration.
