/*
  Warning Panel Test - SK6812B LED Strip Controller
  STM32 Blackpill code to control 1 SK6812B LED strip with 10 LEDs
  Each LED represents a different warning icon/status
  
  Hardware connections (STM32F411CEU6 Blackpill):
  - LED Strip Data Pin: PA7 (Pin A7)
  - LED Strip VCC: 5V or 3.3V
  - LED Strip GND: GND
  
  Note: STM32 Blackpill runs at 3.3V logic level
  If using 5V LED strip, may need level shifter for data line
  
  Warning Panel Layout:
  LED 0: Temperature Warning
  LED 1: Signal Warning
  LED 2: Aircraft Warning
  LED 3: Drone Connection
  LED 4: Warning Left
  LED 5: Warning Right - Same as LED 4
  LED 6: GPS Warning
  LED 7: Connection Warning
  LED 8: Lock Warning
  LED 9: Drone Status
*/

#include <FastLED.h>

// Configuration for STM32 Blackpill
#define NUM_LEDS 10            // 10 LEDs on the strip
#define DATA_PIN PA7           // Data pin for LED strip (PA7 = Pin A7)
#define BRIGHTNESS 100         // 0-255, adjust for desired brightness
#define FRAMES_PER_SECOND 60

// LED array
CRGB leds[NUM_LEDS];

// Warning panel icon definitions
enum WarningIcon {
  TEMPERATURE_WARNING = 0,
  SIGNAL_WARNING= 1,
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
  OFF = 0,
  NORMAL = 1,    // Green
  WARNING = 2,   // Yellow/Orange
  CRITICAL = 3,  // Red
  INFO = 4       // Blue
};

// Current state of each warning icon
WarningState warningStates[NUM_LEDS];

// Flashing configuration for each LED
bool warningFlashing[NUM_LEDS];
unsigned long warningFlashTimer[NUM_LEDS];
bool warningFlashState[NUM_LEDS];
int warningFlashInterval = 500; // Default flash interval in ms

// Custom brightness for each LED (255 = use global brightness)
uint8_t warningBrightness[NUM_LEDS];

// Colors for different warning states
CRGB stateColors[5] = {
  CRGB::Black,     // OFF
  CRGB::Green,     // NORMAL
  CRGB::Orange,    // WARNING
  CRGB::Red,       // CRITICAL
  CRGB::Blue       // INFO
};

// Animation variables for demo mode
unsigned long lastUpdate = 0;
int demoMode = 0;
int demoDelay = 2000; // 2 seconds per demo step
bool demoModeActive = false; // Set to false to disable demo

void setup() {
  Serial.begin(115200);
  Serial.println("STM32 Warning Panel Controller Starting...");
  
  // Initialize LED strip with STM32-specific settings
  FastLED.addLeds<SK6812, DATA_PIN, GRB>(leds, NUM_LEDS);
  
  // Set global brightness
  FastLED.setBrightness(BRIGHTNESS);
  
  // Initialize all warnings to OFF
  for (int i = 0; i < NUM_LEDS; i++) {
    warningStates[i] = OFF;
    warningFlashing[i] = false;
    warningFlashTimer[i] = 0;
    warningFlashState[i] = false;
    warningBrightness[i] = 255; // Use global brightness by default
  }
  
  // Update display
  updateWarningPanel();
  FastLED.show();
  
  Serial.println("STM32 Warning Panel initialized!");
  Serial.println("Hardware: STM32F411CEU6 Blackpill");
  Serial.println("LED Layout:");
  Serial.println("0: Temperature | 1: Signal | 2: Aircraft | 3: Drone Connection | 4&5: Warning L&R");
  Serial.println("6: GPS | 7: Connection | 8: Lock | 9: Drone Status");
  Serial.println("Data Pin: PA7 (A7)");
  Serial.println();
  
  if (demoModeActive) {
    Serial.println("Demo mode active - cycling through warning states");
  } else {
    Serial.println("Ready for serial commands. Type 'help' for command list.");
  }
}

void loop() {
  if (demoModeActive) {
    // Demo mode - cycle through different warning scenarios
    if (millis() - lastUpdate > demoDelay) {
      lastUpdate = millis();
      runDemoMode();
    }
  }
  
  // Check for serial commands
  if (Serial.available()) {
    processSerialCommand();
  }
  
  // Update the warning panel display
  updateWarningPanel();
  FastLED.show();
  
  delay(50); // Small delay for stability
}

// Demo mode to showcase different warning states
void runDemoMode() {
  // Clear all warnings first
  clearAllWarnings();
  
  switch (demoMode) {
    case 0:
      Serial.println("Demo: All systems normal");
      setAllWarningsState(NORMAL);
      break;
      
    case 1:
      Serial.println("Demo: Temperature warning");
      setWarningState(TEMPERATURE_WARNING, WARNING);
      break;
      
    case 2:
      Serial.println("Demo: Signal critical");
      setWarningState(SIGNAL_WARNING, CRITICAL);
      break;
      
    case 3:
      Serial.println("Demo: Aircraft warning");
      setWarningState(AIRCRAFT_WARNING, WARNING);
      break;
      
    case 4:
      Serial.println("Demo: Drone connection lost");
      setWarningState(DRONE_CONNECTION, CRITICAL);
      break;
      
    case 5:
      Serial.println("Demo: Warning indicators active with flashing");
      SetDualWarning(WARNING, true); // Flashing warning
      break;
      
    case 6:
      Serial.println("Demo: GPS warning with custom brightness");
      SetWarning(GPS_WARNING, WARNING, false, 128); // Half brightness
      break;
      
    case 7:
      Serial.println("Demo: Connection critical flashing");
      SetWarning(CONNECTION_WARNING, CRITICAL, true); // Flashing critical
      break;
      
    case 8:
      Serial.println("Demo: Lock warning with slow flash");
      setFlashInterval(1000); // 1 second flash
      SetWarning(LOCK_WARNING, WARNING, true);
      break;
      
    case 9:
      Serial.println("Demo: Drone status info with dim light");
      SetWarning(DRONE_STATUS, INFO, false, 64); // Very dim
      break;
      
    case 10:
      Serial.println("Demo: Multiple warnings with effects");
      setFlashInterval(500); // Reset flash interval
      SetWarning(TEMPERATURE_WARNING, WARNING, true, 200);
      SetWarning(SIGNAL_WARNING, CRITICAL, true);
      SetWarning(GPS_WARNING, WARNING, false, 100);
      SetDualWarning(CRITICAL, true);
      break;
      
    case 11:
      Serial.println("Demo: Custom colors with effects");
      SetWarningCustom(TEMPERATURE_WARNING, CRGB::Purple, true);
      SetWarningCustom(AIRCRAFT_WARNING, CRGB::Cyan, false, 150);
      SetWarningCustom(DRONE_STATUS, CRGB::Yellow, true, 100);
      break;
  }
  
  demoMode = (demoMode + 1) % 12;
}

// Update the physical LEDs based on warning states
void updateWarningPanel() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (warningStates[i] >= 0 && warningStates[i] < 5) {
      CRGB color = stateColors[warningStates[i]];
      
      // Handle flashing
      if (warningFlashing[i] && warningStates[i] != OFF) {
        if (millis() - warningFlashTimer[i] > warningFlashInterval) {
          warningFlashState[i] = !warningFlashState[i];
          warningFlashTimer[i] = millis();
        }
        
        if (!warningFlashState[i]) {
          color = CRGB::Black; // Turn off during flash
        }
      }
      
      // Apply custom brightness if set
      if (warningBrightness[i] < 255) {
        color.nscale8(warningBrightness[i]);
      }
      
      leds[i] = color;
    }
  }
}

// Main function to set warning with all options
void SetWarning(WarningIcon icon, WarningState state, bool flashing = false, uint8_t brightness = 255) {
  if (icon >= 0 && icon < NUM_LEDS) {
    warningStates[icon] = state;
    warningFlashing[icon] = flashing;
    warningBrightness[icon] = brightness;
    
    // Reset flash timer when setting new flashing state
    if (flashing) {
      warningFlashTimer[icon] = millis();
      warningFlashState[icon] = true;
    }
    
    // Handle dual warning indicators (LEDs 4 & 5 are the same)
    if (icon == WARNING_LEFT || icon == WARNING_RIGHT) {
      warningStates[WARNING_LEFT] = state;
      warningStates[WARNING_RIGHT] = state;
      warningFlashing[WARNING_LEFT] = flashing;
      warningFlashing[WARNING_RIGHT] = flashing;
      warningBrightness[WARNING_LEFT] = brightness;
      warningBrightness[WARNING_RIGHT] = brightness;
      
      if (flashing) {
        warningFlashTimer[WARNING_LEFT] = millis();
        warningFlashTimer[WARNING_RIGHT] = millis();
        warningFlashState[WARNING_LEFT] = true;
        warningFlashState[WARNING_RIGHT] = true;
      }
    }
  }
}

// Set warning with custom color (bypasses state colors)
void SetWarningCustom(WarningIcon icon, CRGB color, bool flashing = false, uint8_t brightness = 255) {
  if (icon >= 0 && icon < NUM_LEDS) {
    // Set to a dummy state to indicate custom color
    warningStates[icon] = INFO; // Use INFO state as placeholder
    warningFlashing[icon] = flashing;
    warningBrightness[icon] = brightness;
    
    // Apply brightness to color
    if (brightness < 255) {
      color.nscale8(brightness);
    }
    
    // Set the LED directly
    leds[icon] = color;
    
    // Reset flash timer when setting new flashing state
    if (flashing) {
      warningFlashTimer[icon] = millis();
      warningFlashState[icon] = true;
    }
    
    // Handle dual warning indicators (LEDs 4 & 5 are the same)
    if (icon == WARNING_LEFT || icon == WARNING_RIGHT) {
      warningStates[WARNING_LEFT] = INFO;
      warningStates[WARNING_RIGHT] = INFO;
      warningFlashing[WARNING_LEFT] = flashing;
      warningFlashing[WARNING_RIGHT] = flashing;
      warningBrightness[WARNING_LEFT] = brightness;
      warningBrightness[WARNING_RIGHT] = brightness;
      
      leds[WARNING_LEFT] = color;
      leds[WARNING_RIGHT] = color;
      
      if (flashing) {
        warningFlashTimer[WARNING_LEFT] = millis();
        warningFlashTimer[WARNING_RIGHT] = millis();
        warningFlashState[WARNING_LEFT] = true;
        warningFlashState[WARNING_RIGHT] = true;
      }
    }
  }
}

// Set dual warning with all options
void SetDualWarning(WarningState state, bool flashing = false, uint8_t brightness = 255) {
  SetWarning(WARNING_LEFT, state, flashing, brightness);
  // WARNING_RIGHT will be automatically set by the SetWarning function
}

// Set flash interval for all warnings (in milliseconds)
void setFlashInterval(int interval) {
  warningFlashInterval = interval;
}

// Legacy functions for backward compatibility
void setWarningState(WarningIcon icon, WarningState state) {
  SetWarning(icon, state);
}

// Set dual warning state (affects both LEDs 4 & 5)
void setDualWarningState(WarningState state) {
  SetDualWarning(state);
}

// Set a specific warning to a custom color
void setWarningColor(WarningIcon icon, CRGB color) {
  if (icon >= 0 && icon < NUM_LEDS) {
    leds[icon] = color;
    
    // Handle dual warning indicators (LEDs 4 & 5 are the same)
    if (icon == WARNING_LEFT || icon == WARNING_RIGHT) {
      leds[WARNING_LEFT] = color;
      leds[WARNING_RIGHT] = color;
    }
  }
}

// Set dual warning indicators to custom color
void setDualWarningColor(CRGB color) {
  leds[WARNING_LEFT] = color;
  leds[WARNING_RIGHT] = color;
}

// Clear all warnings (turn off all LEDs)
void clearAllWarnings() {
  for (int i = 0; i < NUM_LEDS; i++) {
    warningStates[i] = OFF;
    warningFlashing[i] = false;
    warningBrightness[i] = 255;
  }
}

// Set all warnings to the same state
void setAllWarningsState(WarningState state) {
  for (int i = 0; i < NUM_LEDS; i++) {
    warningStates[i] = state;
  }
}

// Set all warnings to the same color
void setAllWarningsColor(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
}

// Get current state of a warning
WarningState getWarningState(WarningIcon icon) {
  if (icon >= 0 && icon < NUM_LEDS) {
    return warningStates[icon];
  }
  return OFF;
}

// Check if any warning is in critical state
bool hasCriticalWarnings() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (warningStates[i] == CRITICAL) {
      return true;
    }
  }
  return false;
}

// Check if any warning is active (not OFF)
bool hasActiveWarnings() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (warningStates[i] != OFF) {
      return true;
    }
  }
  return false;
}

// Print current warning status to serial
void printWarningStatus() {
  Serial.println("=== Warning Panel Status ===");
  Serial.print("Temperature: "); Serial.println(warningStates[TEMPERATURE_WARNING]);
  Serial.print("Signal: "); Serial.println(warningStates[SIGNAL_WARNING]);
  Serial.print("Aircraft: "); Serial.println(warningStates[AIRCRAFT_WARNING]);
  Serial.print("Drone Connection: "); Serial.println(warningStates[DRONE_CONNECTION]);
  Serial.print("Warning L&R: "); Serial.println(warningStates[WARNING_LEFT]);
  Serial.print("GPS: "); Serial.println(warningStates[GPS_WARNING]);
  Serial.print("Connection: "); Serial.println(warningStates[CONNECTION_WARNING]);
  Serial.print("Lock: "); Serial.println(warningStates[LOCK_WARNING]);
  Serial.print("Drone Status: "); Serial.println(warningStates[DRONE_STATUS]);
  Serial.println("============================");
}

// Process serial commands
void processSerialCommand() {
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();
  
  if (command == "help" || command == "h") {
    printHelp();
  }
  else if (command == "status" || command == "s") {
    printWarningStatus();
  }
  else if (command == "clear" || command == "c") {
    clearAllWarnings();
    Serial.println("All warnings cleared");
  }
  else if (command == "demo") {
    demoModeActive = !demoModeActive;
    Serial.print("Demo mode: ");
    Serial.println(demoModeActive ? "ON" : "OFF");
    if (!demoModeActive) {
      clearAllWarnings();
    }
  }
  else if (command == "normal") {
    setAllWarningsState(NORMAL);
    Serial.println("All warnings set to NORMAL (green)");
  }
  else if (command.startsWith("set ")) {
    parseSetCommand(command);
  }
  else if (command.startsWith("flash ")) {
    parseFlashCommand(command);
  }
  else if (command.startsWith("bright ")) {
    parseBrightnessCommand(command);
  }
  else if (command.startsWith("color ")) {
    parseColorCommand(command);
  }
  else if (command != "") {
    Serial.println("Unknown command. Type 'help' for available commands.");
  }
}

// Print help information
void printHelp() {
  Serial.println("=== STM32 Warning Panel Commands ===");
  Serial.println("Hardware: STM32F411CEU6 Blackpill");
  Serial.println("Data Pin: PA7 (A7)");
  Serial.println();
  Serial.println("help/h          - Show this help");
  Serial.println("status/s        - Show current status");
  Serial.println("clear/c         - Clear all warnings");
  Serial.println("demo            - Toggle demo mode");
  Serial.println("normal          - Set all to normal (green)");
  Serial.println("");
  Serial.println("SET commands:");
  Serial.println("set <icon> <state>     - Set warning state");
  Serial.println("  Icons: temp, signal, aircraft, drone, warning, gps, connection, lock, status");
  Serial.println("  States: off, normal, warning, critical, info");
  Serial.println("  Example: set temp critical");
  Serial.println("");
  Serial.println("FLASH commands:");
  Serial.println("flash <icon> <on/off>  - Toggle flashing");
  Serial.println("flash speed <ms>       - Set flash interval");
  Serial.println("  Example: flash temp on");
  Serial.println("");
  Serial.println("BRIGHTNESS commands:");
  Serial.println("bright <icon> <0-255>  - Set brightness");
  Serial.println("bright all <0-255>     - Set global brightness");
  Serial.println("  Example: bright temp 128");
  Serial.println("");
  Serial.println("COLOR commands:");
  Serial.println("color <icon> <color>   - Set custom color");
  Serial.println("  Colors: red, green, blue, yellow, purple, cyan, white, orange");
  Serial.println("  Example: color temp red");
  Serial.println("=====================================");
}

// Parse SET command: "set <icon> <state>"
void parseSetCommand(String command) {
  command = command.substring(4); // Remove "set "
  int spaceIndex = command.indexOf(' ');
  
  if (spaceIndex == -1) {
    Serial.println("Usage: set <icon> <state>");
    return;
  }
  
  String iconStr = command.substring(0, spaceIndex);
  String stateStr = command.substring(spaceIndex + 1);
  
  WarningIcon icon = parseIcon(iconStr);
  WarningState state = parseState(stateStr);
  
  if (icon != -1 && state != -1) {
    SetWarning(icon, state);
    Serial.print("Set "); Serial.print(iconStr); Serial.print(" to "); Serial.println(stateStr);
  }
}

// Parse FLASH command: "flash <icon> <on/off>" or "flash speed <ms>"
void parseFlashCommand(String command) {
  command = command.substring(6); // Remove "flash "
  
  if (command.startsWith("speed ")) {
    int interval = command.substring(6).toInt();
    if (interval > 0) {
      setFlashInterval(interval);
      Serial.print("Flash interval set to "); Serial.print(interval); Serial.println("ms");
    } else {
      Serial.println("Invalid interval. Use: flash speed <ms>");
    }
    return;
  }
  
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex == -1) {
    Serial.println("Usage: flash <icon> <on/off> or flash speed <ms>");
    return;
  }
  
  String iconStr = command.substring(0, spaceIndex);
  String flashStr = command.substring(spaceIndex + 1);
  
  WarningIcon icon = parseIcon(iconStr);
  bool flashing = (flashStr == "on" || flashStr == "true" || flashStr == "1");
  
  if (icon != -1) {
    WarningState currentState = getWarningState(icon);
    uint8_t currentBrightness = warningBrightness[icon];
    SetWarning(icon, currentState, flashing, currentBrightness);
    Serial.print("Flash "); Serial.print(iconStr); Serial.print(": "); Serial.println(flashing ? "ON" : "OFF");
  }
}

// Parse BRIGHTNESS command: "bright <icon> <0-255>" or "bright all <0-255>"
void parseBrightnessCommand(String command) {
  command = command.substring(7); // Remove "bright "
  
  if (command.startsWith("all ")) {
    int brightness = command.substring(4).toInt();
    if (brightness >= 0 && brightness <= 255) {
      FastLED.setBrightness(brightness);
      Serial.print("Global brightness set to "); Serial.println(brightness);
    } else {
      Serial.println("Brightness must be 0-255");
    }
    return;
  }
  
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex == -1) {
    Serial.println("Usage: bright <icon> <0-255> or bright all <0-255>");
    return;
  }
  
  String iconStr = command.substring(0, spaceIndex);
  int brightness = command.substring(spaceIndex + 1).toInt();
  
  WarningIcon icon = parseIcon(iconStr);
  
  if (icon != -1 && brightness >= 0 && brightness <= 255) {
    WarningState currentState = getWarningState(icon);
    bool currentFlashing = warningFlashing[icon];
    SetWarning(icon, currentState, currentFlashing, brightness);
    Serial.print("Brightness "); Serial.print(iconStr); Serial.print(": "); Serial.println(brightness);
  } else {
    Serial.println("Invalid icon or brightness (0-255)");
  }
}

// Parse COLOR command: "color <icon> <color>"
void parseColorCommand(String command) {
  command = command.substring(6); // Remove "color "
  int spaceIndex = command.indexOf(' ');
  
  if (spaceIndex == -1) {
    Serial.println("Usage: color <icon> <color>");
    return;
  }
  
  String iconStr = command.substring(0, spaceIndex);
  String colorStr = command.substring(spaceIndex + 1);
  
  WarningIcon icon = parseIcon(iconStr);
  CRGB color = parseColor(colorStr);
  
  if (icon != -1 && color != CRGB::Black) {
    bool currentFlashing = warningFlashing[icon];
    uint8_t currentBrightness = warningBrightness[icon];
    SetWarningCustom(icon, color, currentFlashing, currentBrightness);
    Serial.print("Color "); Serial.print(iconStr); Serial.print(": "); Serial.println(colorStr);
  } else {
    Serial.println("Invalid icon or color");
  }
}

// Parse icon name to enum
WarningIcon parseIcon(String iconStr) {
  if (iconStr == "temp" || iconStr == "temperature") return TEMPERATURE_WARNING;
  if (iconStr == "signal") return SIGNAL_WARNING;
  if (iconStr == "aircraft" || iconStr == "plane") return AIRCRAFT_WARNING;
  if (iconStr == "drone" || iconStr == "connection") return DRONE_CONNECTION;
  if (iconStr == "warning" || iconStr == "warn") return WARNING_LEFT;
  if (iconStr == "gps") return GPS_WARNING;
  if (iconStr == "conn" || iconStr == "communication") return CONNECTION_WARNING;
  if (iconStr == "lock") return LOCK_WARNING;
  if (iconStr == "status" || iconStr == "drone_status") return DRONE_STATUS;
  
  Serial.print("Unknown icon: "); Serial.println(iconStr);
  return (WarningIcon)-1;
}

// Parse state name to enum
WarningState parseState(String stateStr) {
  if (stateStr == "off") return OFF;
  if (stateStr == "normal" || stateStr == "green") return NORMAL;
  if (stateStr == "warning" || stateStr == "yellow" || stateStr == "orange") return WARNING;
  if (stateStr == "critical" || stateStr == "red") return CRITICAL;
  if (stateStr == "info" || stateStr == "blue") return INFO;
  
  Serial.print("Unknown state: "); Serial.println(stateStr);
  return (WarningState)-1;
}

// Parse color name to CRGB
CRGB parseColor(String colorStr) {
  if (colorStr == "red") return CRGB::Red;
  if (colorStr == "green") return CRGB::Green;
  if (colorStr == "blue") return CRGB::Blue;
  if (colorStr == "yellow") return CRGB::Yellow;
  if (colorStr == "purple" || colorStr == "magenta") return CRGB::Purple;
  if (colorStr == "cyan") return CRGB::Cyan;
  if (colorStr == "white") return CRGB::White;
  if (colorStr == "orange") return CRGB::Orange;
  
  Serial.print("Unknown color: "); Serial.println(colorStr);
  return CRGB::Black; // Return black as error indicator
}