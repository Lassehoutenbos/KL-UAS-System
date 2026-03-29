#include "WarningPanel.h"
#include "leds.h"  // Include the existing LED system

// External LED strip defined in leds.cpp
extern Adafruit_NeoPixel strip;

// Global instance
WarningPanel warningPanel;

// Constructor
WarningPanel::WarningPanel()
  : flashInterval(DEFAULT_FLASH_INTERVAL),
    worklightLastFlash(0),
    worklightFlashState(false),
    configMutex(nullptr),
    updateRequired(false) {
  
  // Initialize warning configurations
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    warningConfigs[i] = {WARNING_OFF, false, 255, 0, false};
    lastFlashTime[i] = 0;
    flashState[i] = false;
  }
  
  // Initialize worklight configuration (23 LEDs for worklight)
  worklightConfig = {false, 255, 0, false, 48, 70}; // Use LEDs 48-70 as worklight (23 LEDs)
}


// Destructor
WarningPanel::~WarningPanel() {
  if (configMutex != nullptr) {
    vSemaphoreDelete(configMutex);
  }
}

// Initialize the warning panel
bool WarningPanel::begin() {
  // Create mutex for thread-safe access
  configMutex = xSemaphoreCreateMutex();
  if (configMutex == nullptr) {
    return false;
  }
  
  // Note: LED strip is initialized by setupLeds() in leds.cpp
  // We just use the existing strip instance
  
  // Set default worklight color to white
  worklightConfig.color = strip.Color(255, 255, 255);
  
  updateRequired = true;
  return true;
}

// Main update function - call this regularly in an RTOS task
void WarningPanel::update() {
  if (configMutex == nullptr) return;
  
  // Take mutex with timeout
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    unsigned long currentTime = millis();
    bool needsShow = false;
    
    // Update warning LEDs
    for (int i = 0; i < NUM_WARNING_LEDS; i++) {
      if (warningConfigs[i].flashing && warningConfigs[i].state != WARNING_OFF) {
        if (currentTime - lastFlashTime[i] >= flashInterval) {
          flashState[i] = !flashState[i];
          lastFlashTime[i] = currentTime;
          needsShow = true;
        }
      }
      updateWarningLED(i);
    }
    
    // Update worklight flashing
    if (worklightConfig.flashing && worklightConfig.enabled) {
      if (currentTime - worklightLastFlash >= flashInterval) {
        worklightFlashState = !worklightFlashState;
        worklightLastFlash = currentTime;
        needsShow = true;
      }
    }
    
    // Update worklight LEDs
    updateWorklightLEDs();
    
    // Show changes if needed or if update was explicitly requested
    if (needsShow || updateRequired) {
      strip.show();
      updateRequired = false;
    }
    
    xSemaphoreGive(configMutex);
  }
}

// Set warning with all options
void WarningPanel::setWarning(WarningIcon icon, WarningState state, bool flashing, uint8_t brightness) {
  if (icon < 0 || icon >= NUM_WARNING_LEDS || configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    warningConfigs[icon].state = state;
    warningConfigs[icon].flashing = flashing;
    warningConfigs[icon].brightness = brightness;
    warningConfigs[icon].useCustomColor = false;
    
    // Reset flash timer when setting new flashing state
    if (flashing) {
      lastFlashTime[icon] = millis();
      flashState[icon] = true;
    }
    
    // Handle dual warning indicators (LEDs 4 & 5 are the same)
    if (icon == WARNING_LEFT || icon == WARNING_RIGHT) {
      warningConfigs[WARNING_LEFT] = warningConfigs[icon];
      warningConfigs[WARNING_RIGHT] = warningConfigs[icon];
      
      if (flashing) {
        lastFlashTime[WARNING_LEFT] = millis();
        lastFlashTime[WARNING_RIGHT] = millis();
        flashState[WARNING_LEFT] = true;
        flashState[WARNING_RIGHT] = true;
      }
    }
    
    updateRequired = true;
    xSemaphoreGive(configMutex);
  }
}

// Set warning with custom color
void WarningPanel::setWarningCustom(WarningIcon icon, uint32_t color, bool flashing, uint8_t brightness) {
  if (icon < 0 || icon >= NUM_WARNING_LEDS || configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    warningConfigs[icon].state = WARNING_NORMAL; // Use normal state as base
    warningConfigs[icon].flashing = flashing;
    warningConfigs[icon].brightness = brightness;
    warningConfigs[icon].customColor = color;
    warningConfigs[icon].useCustomColor = true;
    
    // Reset flash timer when setting new flashing state
    if (flashing) {
      lastFlashTime[icon] = millis();
      flashState[icon] = true;
    }
    
    // Handle dual warning indicators
    if (icon == WARNING_LEFT || icon == WARNING_RIGHT) {
      warningConfigs[WARNING_LEFT] = warningConfigs[icon];
      warningConfigs[WARNING_RIGHT] = warningConfigs[icon];
      
      if (flashing) {
        lastFlashTime[WARNING_LEFT] = millis();
        lastFlashTime[WARNING_RIGHT] = millis();
        flashState[WARNING_LEFT] = true;
        flashState[WARNING_RIGHT] = true;
      }
    }
    
    updateRequired = true;
    xSemaphoreGive(configMutex);
  }
}

// Set dual warning indicators to same state
void WarningPanel::setDualWarning(WarningState state, bool flashing, uint8_t brightness) {
  setWarning(WARNING_LEFT, state, flashing, brightness);
}

// Clear specific warning
void WarningPanel::clearWarning(WarningIcon icon) {
  setWarning(icon, WARNING_OFF, false, 255);
}

// Clear all warnings
void WarningPanel::clearAllWarnings() {
  if (configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    for (int i = 0; i < NUM_WARNING_LEDS; i++) {
      warningConfigs[i] = {WARNING_OFF, false, 255, 0, false};
      lastFlashTime[i] = 0;
      flashState[i] = false;
    }
    updateRequired = true;
    xSemaphoreGive(configMutex);
  }
}

// Set all warnings to same state
void WarningPanel::setAllWarningsState(WarningState state) {
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    setWarning((WarningIcon)i, state);
  }
}

// Set flash interval
void WarningPanel::setFlashInterval(int interval) {
  if (interval > 0) {
    flashInterval = interval;
  }
}

// Worklight control functions
void WarningPanel::setWorklight(bool enabled, uint8_t startIndex, uint8_t endIndex, uint8_t brightness, uint32_t color, bool flashing) {
  if (configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    worklightConfig.enabled = enabled;
    worklightConfig.startIndex = startIndex;
    worklightConfig.endIndex = endIndex;
    if (brightness <= 255) worklightConfig.brightness = brightness;
    if (color != 0) worklightConfig.color = color;
    worklightConfig.flashing = flashing;
    
    if (flashing) {
      worklightLastFlash = millis();
      worklightFlashState = true;
    }
    
    updateRequired = true;
    xSemaphoreGive(configMutex);
  }
}

void WarningPanel::setWorklightEnabled(bool enabled) {
  if (configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    worklightConfig.enabled = enabled;
    updateRequired = true;
    xSemaphoreGive(configMutex);
  }
}

void WarningPanel::setWorklightBrightness(uint8_t brightness) {
  if (configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (brightness <= 255) {
      worklightConfig.brightness = brightness;
      updateRequired = true;
    }
    xSemaphoreGive(configMutex);
  }
}

void WarningPanel::setWorklightColor(uint32_t color) {
  if (configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    worklightConfig.color = color;
    updateRequired = true;
    xSemaphoreGive(configMutex);
  }
}

void WarningPanel::setWorklightFlashing(bool flashing) {
  if (configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    worklightConfig.flashing = flashing;
    if (flashing) {
      worklightLastFlash = millis();
      worklightFlashState = true;
    }
    updateRequired = true;
    xSemaphoreGive(configMutex);
  }
}

void WarningPanel::setWorklightRange(uint8_t startIndex, uint8_t endIndex) {
  if (configMutex == nullptr) return;
  
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    worklightConfig.startIndex = startIndex;
    worklightConfig.endIndex = endIndex;
    updateRequired = true;
    xSemaphoreGive(configMutex);
  }
}

// Status query functions
WarningState WarningPanel::getWarningState(WarningIcon icon) {
  if (icon < 0 || icon >= NUM_WARNING_LEDS || configMutex == nullptr) return WARNING_OFF;
  
  WarningState state = WARNING_OFF;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    state = warningConfigs[icon].state;
    xSemaphoreGive(configMutex);
  }
  return state;
}

bool WarningPanel::isWarningFlashing(WarningIcon icon) {
  if (icon < 0 || icon >= NUM_WARNING_LEDS || configMutex == nullptr) return false;
  
  bool flashing = false;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    flashing = warningConfigs[icon].flashing;
    xSemaphoreGive(configMutex);
  }
  return flashing;
}

uint8_t WarningPanel::getWarningBrightness(WarningIcon icon) {
  if (icon < 0 || icon >= NUM_WARNING_LEDS || configMutex == nullptr) return 255;
  
  uint8_t brightness = 255;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    brightness = warningConfigs[icon].brightness;
    xSemaphoreGive(configMutex);
  }
  return brightness;
}

bool WarningPanel::hasCriticalWarnings() {
  if (configMutex == nullptr) return false;
  
  bool hasCritical = false;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (int i = 0; i < NUM_WARNING_LEDS; i++) {
      if (warningConfigs[i].state == WARNING_CRITICAL) {
        hasCritical = true;
        break;
      }
    }
    xSemaphoreGive(configMutex);
  }
  return hasCritical;
}

bool WarningPanel::hasActiveWarnings() {
  if (configMutex == nullptr) return false;
  
  bool hasActive = false;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (int i = 0; i < NUM_WARNING_LEDS; i++) {
      if (warningConfigs[i].state != WARNING_OFF) {
        hasActive = true;
        break;
      }
    }
    xSemaphoreGive(configMutex);
  }
  return hasActive;
}

// Worklight status queries
bool WarningPanel::isWorklightEnabled() {
  if (configMutex == nullptr) return false;
  
  bool enabled = false;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    enabled = worklightConfig.enabled;
    xSemaphoreGive(configMutex);
  }
  return enabled;
}

uint8_t WarningPanel::getWorklightBrightness() {
  if (configMutex == nullptr) return 255;
  
  uint8_t brightness = 255;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    brightness = worklightConfig.brightness;
    xSemaphoreGive(configMutex);
  }
  return brightness;
}

uint32_t WarningPanel::getWorklightColor() {
  if (configMutex == nullptr) return 0;
  
  uint32_t color = 0;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    color = worklightConfig.color;
    xSemaphoreGive(configMutex);
  }
  return color;
}

bool WarningPanel::isWorklightFlashing() {
  if (configMutex == nullptr) return false;
  
  bool flashing = false;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    flashing = worklightConfig.flashing;
    xSemaphoreGive(configMutex);
  }
  return flashing;
}

uint8_t WarningPanel::getWorklightStartIndex() {
  if (configMutex == nullptr) return 0;
  
  uint8_t startIndex = 0;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    startIndex = worklightConfig.startIndex;
    xSemaphoreGive(configMutex);
  }
  return startIndex;
}

uint8_t WarningPanel::getWorklightEndIndex() {
  if (configMutex == nullptr) return 0;
  
  uint8_t endIndex = 0;
  if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    endIndex = worklightConfig.endIndex;
    xSemaphoreGive(configMutex);
  }
  return endIndex;
}

// Private helper methods
uint32_t WarningPanel::getStateColor(WarningState state) {
  switch (state) {
    case WARNING_OFF: return strip.Color(0, 0, 0);
    case WARNING_NORMAL: return strip.Color(0, 255, 0);
    case WARNING_WARNING: return strip.Color(255, 165, 0);
    case WARNING_CRITICAL: return strip.Color(255, 0, 0);
    case WARNING_INFO: return strip.Color(0, 0, 255);
    default: return strip.Color(0, 0, 0);
  }
}

void WarningPanel::updateWarningLED(int index) {
  uint32_t color = 0;
  
  if (warningConfigs[index].useCustomColor) {
    color = warningConfigs[index].customColor;
  } else {
    color = getStateColor(warningConfigs[index].state);
  }
  
  // Handle flashing
  if (warningConfigs[index].flashing && warningConfigs[index].state != WARNING_OFF) {
    if (!flashState[index]) {
      color = strip.Color(0, 0, 0); // Turn off during flash
    }
  }
  
  // Apply brightness
  if (warningConfigs[index].brightness < 255) {
    applyBrightness(color, warningConfigs[index].brightness);
  }
  
  // Set LED at the warning start index + LED index
  strip.setPixelColor(WARNING_LED_START_INDEX + index, color);
}

void WarningPanel::updateWorklightLEDs() {
  uint32_t color = strip.Color(0, 0, 0); // Default to off
  
  if (worklightConfig.enabled) {
    color = worklightConfig.color;
    
    // Handle flashing
    if (worklightConfig.flashing) {
      if (!worklightFlashState) {
        color = strip.Color(0, 0, 0); // Turn off during flash
      }
    }
    
    // Apply brightness
    if (worklightConfig.brightness < 255) {
      applyBrightness(color, worklightConfig.brightness);
    }
  }
  
  // Set worklight LEDs in the specified range
  for (int i = worklightConfig.startIndex; i <= worklightConfig.endIndex; i++) {
    strip.setPixelColor(i, color);
  }
}

void WarningPanel::applyBrightness(uint32_t &color, uint8_t brightness) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  uint8_t w = (color >> 24) & 0xFF;
  
  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;
  w = (w * brightness) / 255;
  
  color = strip.Color(r, g, b, w);
}

// Demo and animation functions
void WarningPanel::runDemoSequence() {
  // Basic demo sequence - can be expanded
  clearAllWarnings();
  vTaskDelay(pdMS_TO_TICKS(500));
  
  setAllWarningsState(WARNING_NORMAL);
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  setWarning(TEMPERATURE_WARNING, WARNING_CRITICAL, true);
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  setDualWarning(WARNING_WARNING, true);
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  // Use the dedicated worklight range (LEDs 34-41)
  setWorklight(true, 34, 41, 255, strip.Color(255, 255, 255), false);
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  setWorklight(true, 34, 37, 200, strip.Color(255, 165, 0), true);
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  clearAllWarnings();
  setWorklightEnabled(false);
}

void WarningPanel::showStartupAnimation() {
  // Simple startup animation
  clearAllWarnings();
  
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    setWarning((WarningIcon)i, WARNING_INFO, false, 100);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  
  // Use a safe range for worklight demo (LEDs 34-37)
  setWorklight(true, 34, 37, 100, strip.Color(255, 255, 255), false);
  vTaskDelay(pdMS_TO_TICKS(500));
  
  for (int i = NUM_WARNING_LEDS - 1; i >= 0; i--) {
    clearWarning((WarningIcon)i);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  
  setWorklightEnabled(false);
}

// Serial command processing (simplified for RTOS environment)
void WarningPanel::processSerialCommand(const String& command) {
  String cmd = command;
  cmd.trim();
  cmd.toLowerCase();
  
  if (cmd == "help" || cmd == "h") {
    printHelp();
  }
  else if (cmd == "status" || cmd == "s") {
    printStatus();
  }
  else if (cmd == "clear" || cmd == "c") {
    clearAllWarnings();
    Serial.println("All warnings cleared");
  }
  else if (cmd == "demo") {
    runDemoSequence();
    Serial.println("Demo sequence completed");
  }
  else if (cmd == "startup") {
    showStartupAnimation();
    Serial.println("Startup animation completed");
  }
  else if (cmd == "normal") {
    setAllWarningsState(WARNING_NORMAL);
    Serial.println("All warnings set to NORMAL");
  }
  else if (cmd.startsWith("worklight on")) {
    setWorklightEnabled(true);
    Serial.println("Worklight ON");
  }
  else if (cmd.startsWith("worklight off")) {
    setWorklightEnabled(false);
    Serial.println("Worklight OFF");
  }
  else if (cmd != "") {
    Serial.println("Unknown command. Type 'help' for command list.");
  }
}

void WarningPanel::printHelp() {
  Serial.println("\n=== RTOS Warning Panel Commands ===");
  Serial.println("Basic Commands:");
  Serial.println("  help, h          - Show this help");
  Serial.println("  status, s        - Show current status");
  Serial.println("  clear, c         - Clear all warnings");
  Serial.println("  demo             - Run demo sequence");
  Serial.println("  startup          - Run startup animation");
  Serial.println("  normal           - Set all to normal (green)");
  Serial.println("  worklight on/off - Control worklight");
  Serial.println();
}

void WarningPanel::printStatus() {
  Serial.println("\n=== Warning Panel Status ===");
  String iconNames[] = {"Temperature", "Signal", "Aircraft", "Drone Connection", 
                        "Warning Left", "Warning Right", "GPS", "Connection", "Lock", "Drone Status"};
  String stateNames[] = {"OFF", "NORMAL", "WARNING", "CRITICAL", "INFO"};
  
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    WarningState state = getWarningState((WarningIcon)i);
    bool flashing = isWarningFlashing((WarningIcon)i);
    uint8_t brightness = getWarningBrightness((WarningIcon)i);
    
    Serial.print(iconNames[i]); Serial.print(": ");
    Serial.print(stateNames[state]);
    if (flashing) Serial.print(" (FLASHING)");
    if (brightness < 255) {
      Serial.print(" (Brightness: "); Serial.print(brightness); Serial.print(")");
    }
    Serial.println();
  }
  
  Serial.println();
  Serial.println("=== Worklight Status ===");
  Serial.print("Enabled: "); Serial.println(isWorklightEnabled() ? "YES" : "NO");
  Serial.print("Brightness: "); Serial.println(getWorklightBrightness());
  Serial.print("Flashing: "); Serial.println(isWorklightFlashing() ? "YES" : "NO");
  Serial.println();
}

// Convenience functions for backward compatibility
void setupWarningPanel() {
  warningPanel.begin();
}

void updateWarningPanel() {
  warningPanel.update();
}
