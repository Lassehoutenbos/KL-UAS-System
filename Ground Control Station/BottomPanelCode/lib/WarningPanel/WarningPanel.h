#ifndef WARNINGPANEL_H
#define WARNINGPANEL_H

// RTOS-friendly Warning Panel controller for LED warning indicators and worklight
// Integrates warning functionality from LedWarningTest into the BottomPanelCode system

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <STM32FreeRTOS.h>

// Warning panel icon definitions
enum WarningIcon {
  TEMPERATURE_WARNING = 0,
  SIGNAL_WARNING = 1,
  AIRCRAFT_WARNING = 2,
  DRONE_CONNECTION = 3,
  WARNING_LEFT = 4,
  WARNING_RIGHT = 5,
  GPS_WARNING = 6,
  CONNECTION_WARNING = 7,
  LOCK_WARNING = 8,
  DRONE_STATUS = 9
};

// Warning states
enum WarningState {
  WARNING_OFF = 0,
  WARNING_NORMAL = 1,    // Green
  WARNING_WARNING = 2,   // Yellow/Orange
  WARNING_CRITICAL = 3,  // Red
  WARNING_INFO = 4       // Blue
};

// Configuration constants
#define NUM_WARNING_LEDS 10        // 10 LEDs for warning indicators
#define WARNING_LED_START_INDEX 38 // Starting index in the main LED strip for warnings (after switch LEDs)
#define WARNING_BRIGHTNESS 100     // 0-255, adjust for desired brightness
#define DEFAULT_FLASH_INTERVAL 500 // Default flash interval in ms

// Forward declaration for external LED strip (defined in leds.cpp)
extern Adafruit_NeoPixel strip;

// Warning panel configuration structure
struct WarningConfig {
  WarningState state;
  bool flashing;
  uint8_t brightness;
  uint32_t customColor;
  bool useCustomColor;
};

// Worklight configuration structure (uses existing LED strip sections)
struct WorklightConfig {
  bool enabled;
  uint8_t brightness;
  uint32_t color;
  bool flashing;
  uint8_t startIndex;  // Start index in the main LED strip
  uint8_t endIndex;    // End index in the main LED strip
};

class WarningPanel {
private:
  // Use external LED strip from leds.cpp (no local strip instance)
  
  // Warning configurations (thread-safe access via mutex)
  WarningConfig warningConfigs[NUM_WARNING_LEDS];
  WorklightConfig worklightConfig;
  
  // Flash control
  int flashInterval;
  unsigned long lastFlashTime[NUM_WARNING_LEDS];
  bool flashState[NUM_WARNING_LEDS];
  unsigned long worklightLastFlash;
  bool worklightFlashState;
  
  // RTOS synchronization
  SemaphoreHandle_t configMutex;
  volatile bool updateRequired;
  
  // Private methods
  uint32_t getStateColor(WarningState state);
  void updateWarningLED(int index);
  void updateWorklightLEDs();
  void applyBrightness(uint32_t &color, uint8_t brightness);
  
public:
  // Constructor
  WarningPanel();
  
  // Initialization (call in setup)
  bool begin();
  
  // Main update function (call in RTOS task)
  void update();
  
  // Warning control functions (thread-safe)
  void setWarning(WarningIcon icon, WarningState state, bool flashing = false, uint8_t brightness = 255);
  void setWarningCustom(WarningIcon icon, uint32_t color, bool flashing = false, uint8_t brightness = 255);
  void setDualWarning(WarningState state, bool flashing = false, uint8_t brightness = 255);
  void clearWarning(WarningIcon icon);
  void clearAllWarnings();
  void setAllWarningsState(WarningState state);
  
  // Flash control
  void setFlashInterval(int interval);
  
  // Worklight control functions (thread-safe)
  void setWorklight(bool enabled, uint8_t startIndex, uint8_t endIndex, uint8_t brightness = 255, uint32_t color = 0, bool flashing = false);
  void setWorklightEnabled(bool enabled);
  void setWorklightBrightness(uint8_t brightness);
  void setWorklightColor(uint32_t color);
  void setWorklightFlashing(bool flashing);
  void setWorklightRange(uint8_t startIndex, uint8_t endIndex);;
  
  // Status query functions (thread-safe)
  WarningState getWarningState(WarningIcon icon);
  bool isWarningFlashing(WarningIcon icon);
  uint8_t getWarningBrightness(WarningIcon icon);
  bool hasCriticalWarnings();
  bool hasActiveWarnings();
  
  // Worklight status queries
  bool isWorklightEnabled();
  uint8_t getWorklightBrightness();
  uint32_t getWorklightColor();
  bool isWorklightFlashing();
  uint8_t getWorklightStartIndex();
  uint8_t getWorklightEndIndex();
  
  // Demo and test functions
  void runDemoSequence();
  void showStartupAnimation();
  
  // Serial command processing (optional, for debugging)
  void processSerialCommand(const String& command);
  void printStatus();
  void printHelp();
  
  // Destructor
  ~WarningPanel();
};

// Global instance (for compatibility)
extern WarningPanel warningPanel;

// Convenience functions for backward compatibility
void setupWarningPanel();
void updateWarningPanel();

#endif // WARNINGPANEL_H
