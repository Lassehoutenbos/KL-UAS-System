/*
 * Example usage of the integrated Warning Panel system
 * This demonstrates how to use the warning panel in your code
 */

#include <WarningPanel.h>
#include <WarningCommunication.h>

// Global instances (automatically created by the library)
// extern WarningPanel warningPanel;
// WarningCommunication warningComm(&warningPanel);

void example_basic_usage() {
    // Initialize the warning panel (done in setup())
    warningPanel.begin();
    
    // Set basic warning states (uses LEDs 0-9 in the main strip)
    warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_NORMAL, false);
    warningPanel.setWarning(GPS_WARNING, WARNING_WARNING, true);  // Flashing warning
    warningPanel.setWarning(DRONE_CONNECTION, WARNING_CRITICAL, true);  // Flashing critical
    
    // Set dual warning indicators
    warningPanel.setDualWarning(WARNING_WARNING, true);
    
    // Control worklight (use LEDs 34-37 as an example)
    uint32_t white = strip.Color(255, 255, 255);
    warningPanel.setWorklight(true, 34, 37, 200, white, false);  // LEDs 34-37, 200 brightness, no flashing
}

void example_custom_colors() {
    // Create custom colors using the external strip
    uint32_t purple = strip.Color(255, 0, 255);
    uint32_t cyan = strip.Color(0, 255, 255);
    uint32_t orange = strip.Color(255, 165, 0);
    
    // Set custom colored warnings
    warningPanel.setWarningCustom(AIRCRAFT_WARNING, purple, true, 150);
    warningPanel.setWarningCustom(SIGNAL_WARNING, cyan, false, 200);
    
    // Set worklight to custom color with specific range
    warningPanel.setWorklight(true, 34, 37, 200, orange, true);
}

void example_worklight_usage() {
    // Control worklight using specific LED indices in the main strip
    // Example: Use LEDs 34-37 for worklight functionality
    
    // Turn on worklight with white color in LEDs 34-37
    uint32_t white = strip.Color(255, 255, 255);
    warningPanel.setWorklight(true, 34, 37, 200, white, false);
    
    // Set worklight to flash orange in a different range
    uint32_t orange = strip.Color(255, 165, 0);
    warningPanel.setWorklight(true, 30, 35, 150, orange, true);
    
    // Change worklight range dynamically
    warningPanel.setWorklightRange(20, 25);
    
    // Turn off worklight
    warningPanel.setWorklightEnabled(false);
}

void example_status_checking() {
    // Check system status
    if (warningPanel.hasCriticalWarnings()) {
        // Handle critical situation
        Serial.println("CRITICAL WARNINGS ACTIVE!");
        
        // Maybe disable certain functions or alert operator
    }
    
    // Check specific warning states
    WarningState tempState = warningPanel.getWarningState(TEMPERATURE_WARNING);
    switch (tempState) {
        case WARNING_CRITICAL:
            Serial.println("Temperature critical!");
            break;
        case WARNING_WARNING:
            Serial.println("Temperature high");
            break;
        case WARNING_NORMAL:
            Serial.println("Temperature normal");
            break;
        case WARNING_OFF:
            Serial.println("Temperature sensor offline");
            break;
    }
    
    // Check if worklight is on
    if (warningPanel.isWorklightEnabled()) {
        uint8_t brightness = warningPanel.getWorklightBrightness();
        Serial.print("Worklight on at ");
        Serial.print(brightness);
        Serial.println(" brightness");
    }
}

void example_temperature_integration() {
    // Example of how to integrate with temperature sensors
    // (This would typically be done in the warning task)
    
    float maxTemp = 0.0f;
    // Assume we have temperature sensors 0-3
    for (int i = 0; i < 4; i++) {
        // float temp = tempSensors.getTemperatureC(i);
        // if (!isnan(temp) && temp > maxTemp) {
        //     maxTemp = temp;
        // }
    }
    
    // Set warning based on temperature
    if (maxTemp > 70.0f) {
        warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_CRITICAL, true);
        Serial.println("CRITICAL TEMPERATURE!");
    } else if (maxTemp > 55.0f) {
        warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_WARNING, false);
    } else if (maxTemp > 20.0f) {
        warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_NORMAL, false);
    } else {
        warningPanel.setWarning(TEMPERATURE_WARNING, WARNING_OFF, false);
    }
}

void example_communication_usage() {
    // Example of using the communication interface
    WarningCommunication warningComm(&warningPanel);
    warningComm.begin();
    
    // Create messages programmatically
    WarningMessage msg = WarningCommunication::createSetWarningMessage(
        GPS_WARNING, WARNING_CRITICAL, true, 255
    );
    
    // In a real application, you would send this message over serial/CAN/I2C
    // Serial.write((uint8_t*)&msg, sizeof(msg));
    
    // Create worklight control message with LED range
    uint32_t orange = strip.Color(255, 165, 0);
    WarningMessage worklightMsg = WarningCommunication::createSetWorklightMessage(
        true, 34, 37, 200, orange, true  // enabled, LEDs 34-37, brightness 200, orange color, flashing
    );
}

void example_demo_and_animations() {
    // Run the built-in demo sequence
    warningPanel.runDemoSequence();
    
    // Show startup animation
    warningPanel.showStartupAnimation();
    
    // Custom animation example
    for (int i = 0; i < NUM_WARNING_LEDS; i++) {
        warningPanel.clearAllWarnings();
        warningPanel.setWarning((WarningIcon)i, WARNING_INFO, false, 100);
        vTaskDelay(pdMS_TO_TICKS(200));  // Wait 200ms
    }
    warningPanel.clearAllWarnings();
}

void example_serial_commands() {
    // Examples of serial commands you can send:
    
    // warn:help           - Show available commands
    // warn:status         - Show current warning states
    // warn:clear          - Clear all warnings
    // warn:demo           - Run demo sequence
    // warn:normal         - Set all warnings to normal (green)
    // warn:worklight on   - Turn worklight on
    // warn:worklight off  - Turn worklight off
    
    // In your code, process commands like this:
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        if (command.startsWith("warn:")) {
            command = command.substring(5);  // Remove "warn:" prefix
            warningPanel.processSerialCommand(command);
        }
    }
}

// RTOS Task example
void warning_task_example(void *pvParameters) {
    // Show startup animation
    warningPanel.showStartupAnimation();
    
    for (;;) {
        // Update the warning panel (handles flashing, etc.)
        warningPanel.update();
        
        // Check system status and update warnings
        example_temperature_integration();
        
        // Process any incoming communication
        // warningComm.update();
        
        // Check for serial commands
        if (Serial.available()) {
            String command = Serial.readStringUntil('\n');
            if (command.startsWith("warn:")) {
                warningPanel.processSerialCommand(command.substring(5));
            }
        }
        
        // Update at 20Hz
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
