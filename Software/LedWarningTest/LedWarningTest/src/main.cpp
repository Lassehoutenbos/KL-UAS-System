#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Configuration for STM32 Blackpill
#define NUM_WARNING_LEDS 10        // 10 LEDs for warning indicators
#define NUM_WORKLIGHT_LEDS 23      // 23 LEDs for worklight strip
#define NUM_LEDS (NUM_WARNING_LEDS + NUM_WORKLIGHT_LEDS)  // Total 33 LEDs
#define DATA_PIN PA7               // Data pin for LED strip (PA7 = Pin A7)
#define BRIGHTNESS 100             // 0-255, adjust for desired brightness

// NeoPixel strip object
Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRBW + NEO_KHZ800);

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
  OFF = 0,
  NORMAL = 1,    // Green
  WARNING = 2,   // Yellow/Orange
  CRITICAL = 3,  // Red
  INFO = 4       // Blue
};

// Current state of each warning icon
WarningState warningStates[NUM_WARNING_LEDS];

// Worklight state variables
bool worklightEnabled = false;
uint8_t worklightBrightness = 255;
uint32_t worklightColor = 0; // Default color (will be set to white in setup)
bool worklightFlashing = false;
unsigned long worklightFlashTimer = 0;
bool worklightFlashState = false;

// Flashing configuration for each LED
bool warningFlashing[NUM_WARNING_LEDS];
unsigned long warningFlashTimer[NUM_WARNING_LEDS];
bool warningFlashState[NUM_WARNING_LEDS];
int warningFlashInterval = 500; // Default flash interval in ms

// Custom brightness for each LED (255 = use global brightness)
uint8_t warningBrightness[NUM_WARNING_LEDS];

// Animation variables for demo mode
unsigned long lastUpdate = 0;
int demoMode = 0;
int demoDelay = 2000; // 2 seconds per demo step
bool demoModeActive = false; // Set to false to disable demo

// Function declarations
void updateWarningPanel();
void updateWorklight();
void SetWarning(WarningIcon icon, WarningState state, bool flashing = false, uint8_t brightness = 255);
void SetWarningCustom(WarningIcon icon, uint32_t color, bool flashing = false, uint8_t brightness = 255);
void SetDualWarning(WarningState state, bool flashing = false, uint8_t brightness = 255);
void setFlashInterval(int interval);
void setWorklight(bool enabled, uint8_t brightness = 255, uint32_t color = 0, bool flashing = false);
void setWorklightBrightness(uint8_t brightness);
void setWorklightColor(uint32_t color);
void setWorklightFlashing(bool flashing);
void runDemoMode();
void processSerialCommand();
void printHelp();
void parseSetCommand(String command);
void parseFlashCommand(String command);
void parseBrightnessCommand(String command);
void parseColorCommand(String command);
void parseWorklightCommand(String command);
WarningIcon parseIcon(String iconStr);
WarningState parseState(String stateStr);
uint32_t parseColor(String colorStr);
void clearAllWarnings();
void setAllWarningsState(WarningState state);
void setAllWarningsColor(uint32_t color);
bool hasCriticalWarnings();
bool hasActiveWarnings();
WarningState getWarningState(WarningIcon icon);
void printWarningStatus();

void setup() {
  Serial.begin(115200);
  Serial.println("STM32 Warning Panel Controller Starting...");
  
  // Initialize NeoPixel strip
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
  
  // Initialize all warnings to OFF
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    warningStates[i] = OFF;
    warningFlashing[i] = false;
    warningFlashTimer[i] = 0;
    warningFlashState[i] = false;
    warningBrightness[i] = 255; // Use global brightness by default
  }
  
  // Initialize worklight
  worklightEnabled = false;
  worklightBrightness = 255;
  worklightColor = strip.Color(255, 255, 255); // Default to white
  worklightFlashing = false;
  worklightFlashTimer = 0;
  worklightFlashState = false;
  
  Serial.println("STM32 Warning Panel initialized!");
  Serial.println("Hardware: STM32F411CEU6 Blackpill");
  Serial.println("LED Layout:");
  Serial.println("0-9: Warning Indicators | 10-32: Worklight Strip (23 LEDs)");
  Serial.println("Warnings: 0:Temp | 1:Signal | 2:Aircraft | 3:Drone | 4&5:Warning L&R");
  Serial.println("6:GPS | 7:Connection | 8:Lock | 9:Status");
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
    if (millis() - lastUpdate > demoDelay) {
      runDemoMode();
      lastUpdate = millis();
    }
  }
  
  // Check for serial commands
  if (Serial.available()) {
    processSerialCommand();
  }
  
  // Update the warning panel display
  updateWarningPanel();
  updateWorklight();
  strip.show();
  
  delay(50); // Small delay for stability
}

// Demo mode to showcase different warning states
void runDemoMode() {
  // Clear all warnings and worklight first
  clearAllWarnings();
  
  switch (demoMode) {
    case 0:
      Serial.println("Demo: All systems normal");
      setAllWarningsState(NORMAL);
      break;
      
    case 1:
      Serial.println("Demo: Temperature critical");
      SetWarning(TEMPERATURE_WARNING, CRITICAL);
      break;
      
    case 2:
      Serial.println("Demo: Signal warning flashing");
      SetWarning(SIGNAL_WARNING, WARNING, true);
      break;
      
    case 3:
      Serial.println("Demo: Aircraft warning");
      SetWarning(AIRCRAFT_WARNING, WARNING);
      break;
      
    case 4:
      Serial.println("Demo: Drone connection critical");
      SetWarning(DRONE_CONNECTION, CRITICAL);
      break;
      
    case 5:
      Serial.println("Demo: Dual warning indicators");
      SetDualWarning(WARNING, true);
      break;
      
    case 6:
      Serial.println("Demo: GPS warning");
      SetWarning(GPS_WARNING, WARNING);
      break;
      
    case 7:
      Serial.println("Demo: Connection critical flashing");
      SetWarning(CONNECTION_WARNING, CRITICAL, true);
      break;
      
    case 8:
      Serial.println("Demo: Lock warning with slow flash");
      setFlashInterval(1000);
      SetWarning(LOCK_WARNING, WARNING, true);
      break;
      
    case 9:
      Serial.println("Demo: Drone status info with dim light");
      SetWarning(DRONE_STATUS, INFO, false, 64);
      break;
      
    case 10:
      Serial.println("Demo: Multiple warnings with effects");
      setFlashInterval(500);
      SetWarning(TEMPERATURE_WARNING, WARNING, true, 200);
      SetWarning(SIGNAL_WARNING, CRITICAL, true);
      SetWarning(GPS_WARNING, WARNING, false, 100);
      SetDualWarning(CRITICAL, true);
      break;
      
    case 11:
      Serial.println("Demo: Custom colors with effects");
      SetWarningCustom(TEMPERATURE_WARNING, strip.Color(255, 0, 255), true); // Purple
      SetWarningCustom(AIRCRAFT_WARNING, strip.Color(0, 255, 255), false, 150); // Cyan
      SetWarningCustom(DRONE_STATUS, strip.Color(255, 255, 0), true, 100); // Yellow
      break;
      
    case 12:
      Serial.println("Demo: Worklight on");
      setWorklight(true, 255, strip.Color(255, 255, 255), false);
      break;
      
    case 13:
      Serial.println("Demo: Worklight flashing orange");
      setWorklight(true, 200, strip.Color(255, 165, 0), true);
      break;
  }
  
  demoMode = (demoMode + 1) % 14;
}

// Update the physical LEDs based on warning states
void updateWarningPanel() {
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    uint32_t color = 0; // Default to off
    
    if (warningStates[i] >= 0 && warningStates[i] < 5) {
      // Get base color for this state
      switch (warningStates[i]) {
        case OFF: color = strip.Color(0, 0, 0); break;
        case NORMAL: color = strip.Color(0, 255, 0); break;
        case WARNING: color = strip.Color(255, 165, 0); break;
        case CRITICAL: color = strip.Color(255, 0, 0); break;
        case INFO: color = strip.Color(0, 0, 255); break;
      }
      
      // Handle flashing
      if (warningFlashing[i] && warningStates[i] != OFF) {
        if (millis() - warningFlashTimer[i] > warningFlashInterval) {
          warningFlashState[i] = !warningFlashState[i];
          warningFlashTimer[i] = millis();
        }
        
        if (!warningFlashState[i]) {
          color = strip.Color(0, 0, 0); // Turn off during flash
        }
      }
      
      // Apply custom brightness if set
      if (warningBrightness[i] < 255) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        r = (r * warningBrightness[i]) / 255;
        g = (g * warningBrightness[i]) / 255;
        b = (b * warningBrightness[i]) / 255;
        
        color = strip.Color(r, g, b);
      }
    }
    
    strip.setPixelColor(i, color);
  }
}

// Update the worklight LEDs
void updateWorklight() {
  uint32_t color = strip.Color(0, 0, 0); // Default to off
  
  if (worklightEnabled) {
    color = worklightColor;
    
    // Handle flashing
    if (worklightFlashing) {
      if (millis() - worklightFlashTimer > warningFlashInterval) {
        worklightFlashState = !worklightFlashState;
        worklightFlashTimer = millis();
      }
      
      if (!worklightFlashState) {
        color = strip.Color(0, 0, 0); // Turn off during flash
      }
    }
    
    // Apply brightness
    if (worklightBrightness < 255) {
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;
      
      r = (r * worklightBrightness) / 255;
      g = (g * worklightBrightness) / 255;
      b = (b * worklightBrightness) / 255;
      
      color = strip.Color(r, g, b);
    }
  }
  
  // Set all worklight LEDs to the same color
  for (int i = NUM_WARNING_LEDS; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
}

// Worklight control functions
void setWorklight(bool enabled, uint8_t brightness, uint32_t color, bool flashing) {
  worklightEnabled = enabled;
  if (brightness <= 255) worklightBrightness = brightness;
  if (color != 0) worklightColor = color;
  worklightFlashing = flashing;
  
  if (flashing) {
    worklightFlashTimer = millis();
    worklightFlashState = true;
  }
}

void setWorklightBrightness(uint8_t brightness) {
  if (brightness <= 255) {
    worklightBrightness = brightness;
  }
}

void setWorklightColor(uint32_t color) {
  worklightColor = color;
}

void setWorklightFlashing(bool flashing) {
  worklightFlashing = flashing;
  if (flashing) {
    worklightFlashTimer = millis();
    worklightFlashState = true;
  }
}

// Main function to set warning with all options
void SetWarning(WarningIcon icon, WarningState state, bool flashing, uint8_t brightness) {
  if (icon >= 0 && icon < NUM_WARNING_LEDS) {
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
void SetWarningCustom(WarningIcon icon, uint32_t color, bool flashing, uint8_t brightness) {
  if (icon >= 0 && icon < NUM_WARNING_LEDS) {
    warningFlashing[icon] = flashing;
    warningBrightness[icon] = brightness;
    
    // Reset flash timer when setting new flashing state
    if (flashing) {
      warningFlashTimer[icon] = millis();
      warningFlashState[icon] = true;
    }
    
    // Apply custom brightness if set
    if (brightness < 255) {
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;
      
      r = (r * brightness) / 255;
      g = (g * brightness) / 255;
      b = (b * brightness) / 255;
      
      color = strip.Color(r, g, b);
    }
    
    strip.setPixelColor(icon, color);
    
    // Handle dual warning indicators (LEDs 4 & 5 are the same)
    if (icon == WARNING_LEFT || icon == WARNING_RIGHT) {
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
      
      strip.setPixelColor(WARNING_LEFT, color);
      strip.setPixelColor(WARNING_RIGHT, color);
    }
  }
}

// Set dual warning indicators (LEDs 4 & 5) to same state
void SetDualWarning(WarningState state, bool flashing, uint8_t brightness) {
  SetWarning(WARNING_LEFT, state, flashing, brightness);
  // WARNING_RIGHT is automatically handled in SetWarning function
}

// Set flash interval for all flashing LEDs
void setFlashInterval(int interval) {
  warningFlashInterval = interval;
}

// Serial command processing
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
    Serial.println("All warnings set to NORMAL");
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
  else if (command.startsWith("worklight ") || command.startsWith("work ")) {
    parseWorklightCommand(command);
  }
  else if (command != "") {
    Serial.println("Unknown command. Type 'help' for command list.");
  }
}

// Print help information
void printHelp() {
  Serial.println("\n=== NeoPixel Warning Panel Commands ===");
  Serial.println("Basic Commands:");
  Serial.println("  help, h          - Show this help");
  Serial.println("  status, s        - Show current status");
  Serial.println("  clear, c         - Clear all warnings");
  Serial.println("  demo             - Toggle demo mode");
  Serial.println("  normal           - Set all to normal (green)");
  Serial.println();
  Serial.println("Set Commands:");
  Serial.println("  set <icon> <state>");
  Serial.println("  Icons: temp, signal, aircraft, drone, warning, gps, connection, lock, status");
  Serial.println("  States: off, normal, warning, critical, info");
  Serial.println("  Example: set temp critical");
  Serial.println();
  Serial.println("Flash Commands:");
  Serial.println("  flash <icon> <on/off> - Toggle flashing");
  Serial.println("  flash speed <ms>      - Set flash interval");
  Serial.println("  Example: flash temp on");
  Serial.println();
  Serial.println("Brightness Commands:");
  Serial.println("  bright <icon> <0-255> - Set LED brightness");
  Serial.println("  bright all <0-255>    - Set global brightness");
  Serial.println("  Example: bright temp 128");
  Serial.println();
  Serial.println("Color Commands:");
  Serial.println("  color <icon> <color>");
  Serial.println("  Colors: red, green, blue, yellow, purple, cyan, white, orange");
  Serial.println("  Example: color temp purple");
  Serial.println();
  Serial.println("Worklight Commands:");
  Serial.println("  worklight on/off          - Turn worklight on/off");
  Serial.println("  worklight brightness <0-255> - Set worklight brightness");
  Serial.println("  worklight color <color>   - Set worklight color");
  Serial.println("  worklight flash on/off    - Toggle worklight flashing");
  Serial.println("  Example: worklight on, work brightness 200");
  Serial.println();
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
  } else {
    Serial.println("Invalid icon or state");
  }
}

// Parse FLASH command: "flash <icon> <on/off>" or "flash speed <ms>"
void parseFlashCommand(String command) {
  command = command.substring(6); // Remove "flash "
  
  if (command.startsWith("speed ")) {
    int speed = command.substring(6).toInt();
    if (speed > 0) {
      setFlashInterval(speed);
      Serial.print("Flash interval set to "); Serial.print(speed); Serial.println("ms");
    } else {
      Serial.println("Invalid speed value");
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
  } else {
    Serial.println("Invalid icon");
  }
}

// Parse BRIGHTNESS command: "bright <icon> <0-255>" or "bright all <0-255>"
void parseBrightnessCommand(String command) {
  command = command.substring(7); // Remove "bright "
  
  if (command.startsWith("all ")) {
    int brightness = command.substring(4).toInt();
    if (brightness >= 0 && brightness <= 255) {
      strip.setBrightness(brightness);
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
  uint32_t color = parseColor(colorStr);
  
  if (icon != -1 && color != 0) {
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
  if (iconStr == "temp" || iconStr == "temperature") return TEMPERATURE_WARNING;     // LED 0
  if (iconStr == "signal") return SIGNAL_WARNING;                                   // LED 1
  if (iconStr == "aircraft" || iconStr == "plane") return AIRCRAFT_WARNING;        // LED 2
  if (iconStr == "drone") return DRONE_CONNECTION;                                  // LED 3
  if (iconStr == "warning" || iconStr == "warn") return WARNING_LEFT;              // LED 4
  if (iconStr == "gps") return GPS_WARNING;                                        // LED 6
  if (iconStr == "connection" || iconStr == "conn") return CONNECTION_WARNING;     // LED 7
  if (iconStr == "lock") return LOCK_WARNING;                                      // LED 8
  if (iconStr == "status" || iconStr == "drone_status") return DRONE_STATUS;       // LED 9
  return (WarningIcon)-1;
}

// Parse state name to enum
WarningState parseState(String stateStr) {
  if (stateStr == "off" || stateStr == "0") return OFF;
  if (stateStr == "normal" || stateStr == "green" || stateStr == "1") return NORMAL;
  if (stateStr == "warning" || stateStr == "orange" || stateStr == "2") return WARNING;
  if (stateStr == "critical" || stateStr == "red" || stateStr == "3") return CRITICAL;
  if (stateStr == "info" || stateStr == "blue" || stateStr == "4") return INFO;
  return (WarningState)-1;
}

// Parse color name to 32-bit color value
uint32_t parseColor(String colorStr) {
  if (colorStr == "red") return strip.Color(255, 0, 0);
  if (colorStr == "green") return strip.Color(0, 255, 0);
  if (colorStr == "blue") return strip.Color(0, 0, 255);
  if (colorStr == "yellow") return strip.Color(255, 255, 0);
  if (colorStr == "purple" || colorStr == "magenta") return strip.Color(255, 0, 255);
  if (colorStr == "cyan") return strip.Color(0, 255, 255);
  if (colorStr == "white") return strip.Color(255, 255, 255);
  if (colorStr == "orange") return strip.Color(255, 165, 0);
  return 0; // Invalid color
}

// Parse WORKLIGHT command: "worklight <action> [value]"
void parseWorklightCommand(String command) {
  // Remove "worklight " or "work " prefix
  if (command.startsWith("worklight ")) {
    command = command.substring(10);
  } else if (command.startsWith("work ")) {
    command = command.substring(5);
  }
  
  if (command == "on" || command == "1" || command == "true") {
    setWorklight(true, worklightBrightness, worklightColor, worklightFlashing);
    Serial.println("Worklight: ON");
  }
  else if (command == "off" || command == "0" || command == "false") {
    setWorklight(false, worklightBrightness, worklightColor, worklightFlashing);
    Serial.println("Worklight: OFF");
  }
  else if (command.startsWith("brightness ") || command.startsWith("bright ")) {
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex != -1) {
      int brightness = command.substring(spaceIndex + 1).toInt();
      if (brightness >= 0 && brightness <= 255) {
        setWorklightBrightness(brightness);
        Serial.print("Worklight brightness: "); Serial.println(brightness);
      } else {
        Serial.println("Brightness must be 0-255");
      }
    } else {
      Serial.println("Usage: worklight brightness <0-255>");
    }
  }
  else if (command.startsWith("color ")) {
    String colorStr = command.substring(6);
    uint32_t color = parseColor(colorStr);
    if (color != 0) {
      setWorklightColor(color);
      Serial.print("Worklight color: "); Serial.println(colorStr);
    } else {
      Serial.println("Invalid color");
    }
  }
  else if (command.startsWith("flash ")) {
    String flashStr = command.substring(6);
    bool flashing = (flashStr == "on" || flashStr == "true" || flashStr == "1");
    setWorklightFlashing(flashing);
    Serial.print("Worklight flash: "); Serial.println(flashing ? "ON" : "OFF");
  }
  else if (command == "status") {
    Serial.println("\n=== Worklight Status ===");
    Serial.print("Enabled: "); Serial.println(worklightEnabled ? "YES" : "NO");
    Serial.print("Brightness: "); Serial.println(worklightBrightness);
    Serial.print("Flashing: "); Serial.println(worklightFlashing ? "YES" : "NO");
    uint8_t r = (worklightColor >> 16) & 0xFF;
    uint8_t g = (worklightColor >> 8) & 0xFF;
    uint8_t b = worklightColor & 0xFF;
    Serial.print("Color (RGB): "); Serial.print(r); Serial.print(","); 
    Serial.print(g); Serial.print(","); Serial.println(b);
    Serial.println();
  }
  else {
    Serial.println("Worklight commands: on/off, brightness <0-255>, color <color>, flash on/off, status");
  }
}

// Utility functions
void clearAllWarnings() {
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    warningStates[i] = OFF;
    warningFlashing[i] = false;
    warningBrightness[i] = 255;
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  
  // Also turn off worklight
  setWorklight(false, 255, strip.Color(255, 255, 255), false);
}

void setAllWarningsState(WarningState state) {
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    SetWarning((WarningIcon)i, state);
  }
}

void setAllWarningsColor(uint32_t color) {
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    SetWarningCustom((WarningIcon)i, color);
  }
}

bool hasCriticalWarnings() {
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    if (warningStates[i] == CRITICAL) return true;
  }
  return false;
}

bool hasActiveWarnings() {
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    if (warningStates[i] != OFF) return true;
  }
  return false;
}

WarningState getWarningState(WarningIcon icon) {
  if (icon >= 0 && icon < NUM_WARNING_LEDS) {
    return warningStates[icon];
  }
  return OFF;
}

void printWarningStatus() {
  Serial.println("\n=== Warning Panel Status ===");
  String iconNames[] = {"Temperature", "Signal", "Aircraft", "Drone Connection", 
                        "Warning Left", "Warning Right", "GPS", "Connection", "Lock", "Drone Status"};
  String stateNames[] = {"OFF", "NORMAL", "WARNING", "CRITICAL", "INFO"};
  
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    Serial.print(iconNames[i]); Serial.print(": ");
    Serial.print(stateNames[warningStates[i]]);
    if (warningFlashing[i]) Serial.print(" (FLASHING)");
    if (warningBrightness[i] < 255) {
      Serial.print(" (Brightness: "); Serial.print(warningBrightness[i]); Serial.print(")");
    }
    Serial.println();
  }
  
  Serial.println();
  Serial.println("=== Worklight Status ===");
  Serial.print("Enabled: "); Serial.println(worklightEnabled ? "YES" : "NO");
  Serial.print("Brightness: "); Serial.println(worklightBrightness);
  Serial.print("Flashing: "); Serial.println(worklightFlashing ? "YES" : "NO");
  uint8_t r = (worklightColor >> 16) & 0xFF;
  uint8_t g = (worklightColor >> 8) & 0xFF;
  uint8_t b = worklightColor & 0xFF;
  Serial.print("Color (RGB): "); Serial.print(r); Serial.print(","); 
  Serial.print(g); Serial.print(","); Serial.println(b);
  Serial.println();
}