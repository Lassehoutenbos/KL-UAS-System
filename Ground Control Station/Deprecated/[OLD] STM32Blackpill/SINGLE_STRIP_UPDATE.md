# Updated Warning Panel Integration - Single LED Strip

## Overview of Changes

The Warning Panel system has been **updated** to use the **existing single LED strip** connected to **PB3** (as defined in the existing `leds.cpp`), rather than creating a separate strip on PA7. This provides better hardware integration and eliminates potential conflicts.

## Key Changes Made

### 1. Hardware Integration
- **Removed**: Separate LED strip on PA7
- **Using**: Existing LED strip on PB3 (38 LEDs total from `leds.cpp`)
- **Pin**: PB3 (as defined in existing `leds.cpp`)
- **Strip Instance**: Uses `extern Adafruit_NeoPixel strip` from `leds.cpp`

### 2. LED Allocation Strategy
- **Warning LEDs**: Use indices 0-9 (first 10 LEDs in the main strip)
- **Worklight LEDs**: Configurable range (default: 34-37, can be customized)
- **Switch LEDs**: Remain unchanged (as per existing `ledMap` in `leds.cpp`)

### 3. Code Changes

#### WarningPanel.h
```cpp
// Removed separate strip definition
#define WARNING_LED_START_INDEX 0  // Starting index in main strip
extern Adafruit_NeoPixel strip;   // Use external strip

// Updated worklight config to include LED range
struct WorklightConfig {
  bool enabled;
  uint8_t brightness;
  uint32_t color;
  bool flashing;
  uint8_t startIndex;  // Start index in main strip
  uint8_t endIndex;    // End index in main strip
};

// Updated worklight control functions
void setWorklight(bool enabled, uint8_t startIndex, uint8_t endIndex, 
                 uint8_t brightness = 255, uint32_t color = 0, bool flashing = false);
void setWorklightRange(uint8_t startIndex, uint8_t endIndex);
```

#### WarningPanel.cpp
```cpp
#include "leds.h"  // Include existing LED system
extern Adafruit_NeoPixel strip;  // Use external strip

// Constructor no longer creates separate strip
WarningPanel::WarningPanel() : flashInterval(DEFAULT_FLASH_INTERVAL), ... {
  // Default worklight range: LEDs 34-37
  worklightConfig = {false, 255, 0, false, 34, 37};
}

// begin() doesn't initialize strip (done by setupLeds())
bool WarningPanel::begin() {
  // Strip already initialized by setupLeds()
  configMutex = xSemaphoreCreateMutex();
  // ...
}

// LED updates use main strip with offset
void updateWarningLED(int index) {
  // Set LED at WARNING_LED_START_INDEX + index
  strip.setPixelColor(WARNING_LED_START_INDEX + index, color);
}

void updateWorklightLEDs() {
  // Use configurable range
  for (int i = worklightConfig.startIndex; i <= worklightConfig.endIndex; i++) {
    strip.setPixelColor(i, color);
  }
}
```

#### main.cpp
```cpp
void setup() {
  // ...
  setupLeds();          // Initialize LED system FIRST
  warningPanel.begin(); // Then initialize warning panel
  // ...
}
```

### 4. LED Mapping Strategy

With 38 LEDs total in the main strip (as per `leds.cpp`):

```
LEDs 0-9:    Warning Panel indicators (10 LEDs)
LEDs 10-33:  Switch LEDs (existing system, as per ledMap)
LEDs 34-37:  Default worklight range (4 LEDs)
LEDs 38+:    Available for future expansion
```

### 5. Integration Benefits

#### Hardware Benefits
✅ **Single LED strip** - No need for additional hardware  
✅ **Existing pin usage** - Uses PB3 (already defined)  
✅ **No conflicts** - Works with existing switch LEDs  
✅ **Shared power supply** - Uses existing 5V rail  

#### Software Benefits
✅ **Code reuse** - Leverages existing `leds.cpp` initialization  
✅ **Memory efficiency** - No duplicate strip instances  
✅ **Thread safety** - Single strip access point  
✅ **Flexibility** - Configurable LED ranges  

### 6. Usage Examples

#### Basic Warning Control
```cpp
// Warning LEDs use indices 0-9 automatically
warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_CRITICAL, true);
warningPanel.setDualWarning(WARNING_WARNING, true);
```

#### Worklight Control with Range
```cpp
// Use specific LED range for worklight
uint32_t white = strip.Color(255, 255, 255);
warningPanel.setWorklight(true, 34, 37, 200, white, false); // LEDs 34-37

// Change worklight range dynamically
warningPanel.setWorklightRange(30, 35); // Use LEDs 30-35 instead
```

#### System Integration
```cpp
void warningTask(void *) {
  for (;;) {
    // Update only warning panel LEDs (doesn't affect switch LEDs)
    warningPanel.update();
    
    // Existing switch LEDs continue to work via leds.cpp
    setLed(3, {255, 0, 0, 0}, true); // Switch LED still works
    updateLeds(); // Update switch LEDs
    
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
```

### 7. Coexistence with Existing System

The warning panel now **coexists peacefully** with the existing LED system:

- **Switch LEDs**: Continue to work via `setLed()` and `updateLeds()` from `leds.cpp`
- **Warning LEDs**: Controlled by `warningPanel.setWarning()` (uses LEDs 0-9)
- **Worklight**: Controlled by `warningPanel.setWorklight()` (configurable range)
- **No interference**: Each system controls different LED ranges

### 8. Migration from Previous Version

If you were using the previous version with PA7:

#### Old Code
```cpp
// Previous version (PA7, separate strip)
warningPanel.setWorklight(true, 255, white, false);
```

#### New Code
```cpp
// New version (PB3, shared strip with range)
uint32_t white = strip.Color(255, 255, 255);
warningPanel.setWorklight(true, 34, 37, 255, white, false);
```

### 9. Configuration Options

#### Default LED Allocation
```cpp
#define WARNING_LED_START_INDEX 0    // Warnings start at LED 0
// Worklight default range: 34-37 (configurable at runtime)
```

#### Custom LED Ranges
```cpp
// Use different worklight range
warningPanel.setWorklightRange(20, 25);  // Use LEDs 20-25

// Use different warning start index (if needed)
// #define WARNING_LED_START_INDEX 10  // Start warnings at LED 10
```

### 10. Error Handling

The system includes proper bounds checking:
- Warning LEDs are limited to the defined range
- Worklight ranges are validated before use
- Strip updates only occur when changes are made
- Mutex timeouts prevent deadlocks

## Summary

The updated Warning Panel system now:

1. **Uses the existing LED strip** on PB3 (38 LEDs)
2. **Allocates LEDs efficiently**: 0-9 for warnings, 34-37 for worklight (default)
3. **Maintains compatibility** with existing switch LED system
4. **Provides flexible worklight control** with configurable LED ranges
5. **Eliminates hardware conflicts** and reduces component count
6. **Maintains RTOS-friendly operation** with proper synchronization

This approach provides a **cleaner, more integrated solution** that works seamlessly with the existing BottomPanelCode system while providing all the warning functionality from the original LedWarningTest.
