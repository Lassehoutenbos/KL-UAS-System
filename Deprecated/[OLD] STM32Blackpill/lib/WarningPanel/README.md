# Warning Panel Integration

This document describes the integration of the LED Warning Test functionality into the BottomPanelCode codebase, making it RTOS-friendly.

## Overview

The LED warning functionality from the LedWarningTest project has been integrated into the BottomPanelCode as a new library called `WarningPanel`. This integration provides:

- RTOS-compatible warning LED control
- Thread-safe operation using FreeRTOS mutexes
- Integration with existing temperature and switch monitoring
- Communication protocol for external control
- Backward compatibility with existing LED system

## Architecture

### Files Added/Modified

1. **lib/WarningPanel/WarningPanel.h** - Main warning panel class header
2. **lib/WarningPanel/WarningPanel.cpp** - Implementation of warning panel functionality
3. **lib/WarningPanel/WarningCommunication.h** - Communication protocol header
4. **lib/WarningPanel/WarningCommunication.cpp** - Communication protocol implementation
5. **src/main.cpp** - Modified to include warning panel task and integration

### Key Features

#### Thread-Safe Operation
- All warning panel operations are protected by FreeRTOS mutexes
- Safe to call from multiple RTOS tasks
- Non-blocking operations with timeouts

#### Warning Icons
- `TEMPERATURE_WARNING` (0) - Temperature monitoring
- `SIGNAL_WARNING` (1) - Signal strength/quality
- `AIRCRAFT_WARNING` (2) - Aircraft status
- `DRONE_CONNECTION` (3) - Drone connection status
- `WARNING_LEFT` (4) - Left warning indicator
- `WARNING_RIGHT` (5) - Right warning indicator (linked with left)
- `GPS_WARNING` (6) - GPS status
- `CONNECTION_WARNING` (7) - General connection status
- `LOCK_WARNING` (8) - System lock status
- `DRONE_STATUS` (9) - Overall drone status

#### Warning States
- `WARNING_OFF` (0) - LED off
- `WARNING_NORMAL` (1) - Green (normal operation)
- `WARNING_WARNING` (2) - Orange/Yellow (warning condition)
- `WARNING_CRITICAL` (3) - Red (critical condition)
- `WARNING_INFO` (4) - Blue (information)

## Hardware Configuration

### LED Layout
- LEDs 0-9: Warning indicators (10 LEDs)
- LEDs 10-32: Worklight strip (23 LEDs)
- Total: 33 LEDs on PA7 data pin

### Pin Configuration
- Data Pin: PA7 (configured as WARNING_DATA_PIN)
- LED Type: SK6812 RGBW (NEO_GRBW + NEO_KHZ800)
- Default Brightness: 100 (0-255 range)

## Usage Examples

### Basic Warning Control
```cpp
// Set temperature warning to critical with flashing
warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_CRITICAL, true);

// Set dual warning indicators to warning state
warningPanel.setDualWarning(WARNING_WARNING, true);

// Clear a specific warning
warningPanel.clearWarning(GPS_WARNING);

// Clear all warnings
warningPanel.clearAllWarnings();
```

### Custom Colors
```cpp
// Set custom purple color with flashing
uint32_t purple = warningPanel.strip.Color(255, 0, 255);
warningPanel.setWarningCustom(SIGNAL_WARNING, purple, true, 200);
```

### Worklight Control
```cpp
// Turn on worklight with white color
warningPanel.setWorklight(true, 255, warningPanel.strip.Color(255, 255, 255), false);

// Set worklight to flash orange
warningPanel.setWorklight(true, 200, warningPanel.strip.Color(255, 165, 0), true);
```

### Status Queries
```cpp
// Check if any warnings are critical
if (warningPanel.hasCriticalWarnings()) {
    // Handle critical situation
}

// Get specific warning state
WarningState tempState = warningPanel.getWarningState(TEMPERATURE_WARNING);
```

## RTOS Integration

### Task Structure
The warning panel runs in its own RTOS task (`warningTask`) with:
- Priority: 1 (low priority)
- Stack size: 512 bytes
- Update rate: 20Hz (50ms delay)

### Task Responsibilities
1. Update LED states and handle flashing
2. Process communication messages
3. Integrate with system status (temperature, switches)
4. Handle serial commands for debugging

### Thread Safety
All public methods use mutexes with timeouts to ensure thread-safe operation:
```cpp
if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // Modify configuration
    xSemaphoreGive(configMutex);
}
```

## Communication Protocol

### Serial Commands
Simple text commands prefixed with "warn:":
```
warn:help          - Show help
warn:status        - Show current status
warn:clear         - Clear all warnings
warn:demo          - Run demo sequence
warn:normal        - Set all to normal
warn:worklight on  - Turn worklight on
warn:worklight off - Turn worklight off
```

### Binary Protocol
For external systems, a binary protocol is available via `WarningCommunication`:

#### Message Types
- `MSG_SET_WARNING` (0x01) - Set warning state
- `MSG_SET_WARNING_CUSTOM` (0x02) - Set custom color
- `MSG_SET_WORKLIGHT` (0x03) - Control worklight
- `MSG_CLEAR_WARNING` (0x04) - Clear specific warning
- `MSG_CLEAR_ALL` (0x05) - Clear all warnings
- `MSG_GET_STATUS` (0x10) - Request status
- `MSG_PING` (0xFF) - Ping/connectivity test

#### Message Structure
```cpp
struct WarningMessage {
  uint8_t type;       // Message type
  uint8_t icon;       // Warning icon ID
  uint8_t state;      // Warning state
  uint8_t flags;      // bit 0: flashing, others reserved
  uint8_t brightness; // 0-255
  uint32_t color;     // RGBW color for custom colors
  uint16_t interval;  // Flash interval for timing control
} __attribute__((packed));
```

## Integration with Existing System

### Temperature Integration
The warning panel automatically monitors temperature sensors:
```cpp
// Temperature thresholds
if (maxTemp > 60.0f) {
    // Critical temperature - red flashing
    warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_CRITICAL, true);
} else if (maxTemp > 45.0f) {
    // Warning temperature - orange solid
    warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_WARNING, false);
}
```

### Switch Integration
Integration with existing switch monitoring:
```cpp
// Lock warning
if (Switches::isLocked) {
    warningPanel.setWarning(LOCK_WARNING, WARNING_CRITICAL, false);
}

// Switch position warnings
if (!Switches::isConfirmed) {
    warningPanel.setDualWarning(WARNING_WARNING, true);
}
```

### Coexistence with Existing LEDs
The warning panel operates independently of the existing LED system:
- Uses different pin (PA7 vs PB3)
- Separate LED strip for warnings
- Existing switch LEDs continue to work normally
- No conflicts with existing `leds.h/cpp` functionality

## Performance Considerations

### Memory Usage
- WarningPanel class: ~200 bytes
- RTOS task stack: 512 bytes
- Mutex overhead: ~32 bytes
- Total additional RAM: ~750 bytes

### CPU Usage
- 20Hz update rate with minimal processing
- Mutex operations are very fast
- LED updates only when changes occur
- Estimated CPU usage: <1%

### Power Consumption
- 33 LEDs at full brightness: ~2A @ 5V
- Typical usage (few warnings): ~200mA @ 5V
- Worklight adds significant current when active

## Troubleshooting

### Common Issues

1. **LEDs not responding**
   - Check PA7 pin connection
   - Verify 5V power supply
   - Ensure proper ground connection

2. **Flashing not working**
   - Verify flash interval is reasonable (>50ms)
   - Check that warning state is not OFF
   - Ensure task is running properly

3. **Communication issues**
   - Verify serial baud rate (115200)
   - Check message structure alignment
   - Use text commands for debugging

### Debug Commands
```cpp
// Print current status
warningPanel.printStatus();

// Test all LEDs
warningPanel.runDemoSequence();

// Check communication
warningComm.sendStatusUpdate();
```

## Future Enhancements

### Possible Improvements
1. **CAN/I2C Communication** - Add support for other communication protocols
2. **Audio Integration** - Add buzzer support for critical warnings
3. **Pattern Library** - Predefined animation patterns
4. **Configuration Storage** - EEPROM/Flash storage for settings
5. **Power Management** - Automatic brightness adjustment based on ambient light

### Extension Points
The system is designed to be easily extensible:
- Add new warning icons by extending the enum
- Create custom animation patterns
- Implement additional communication protocols
- Add environmental sensors integration

## Conclusion

The integrated warning panel system provides a robust, RTOS-friendly solution for LED warning management while maintaining compatibility with the existing BottomPanelCode system. The thread-safe design ensures reliable operation in the multi-tasking environment, and the communication protocol allows for easy integration with external systems.
