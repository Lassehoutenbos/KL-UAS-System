#ifndef WARNINGCOMMUNICATION_H
#define WARNINGCOMMUNICATION_H

// Communication interface for the warning panel system
// Allows external systems to control warning states via serial/I2C/CAN

#include <Arduino.h>
#include <WarningPanel.h>

// Message types for warning communication
enum WarningMessageType {
  MSG_SET_WARNING = 0x01,
  MSG_SET_WARNING_CUSTOM = 0x02,
  MSG_SET_WORKLIGHT = 0x03,
  MSG_CLEAR_WARNING = 0x04,
  MSG_CLEAR_ALL = 0x05,
  MSG_SET_FLASH_INTERVAL = 0x06,
  MSG_GET_STATUS = 0x10,
  MSG_PING = 0xFF
};

// Warning message structure
struct WarningMessage {
  uint8_t type;
  uint8_t icon;
  uint8_t state;
  uint8_t flags;  // bit 0: flashing, bit 1-7: reserved
  uint8_t brightness;
  uint8_t startIndex; // for worklight range
  uint8_t endIndex;   // for worklight range  
  uint32_t color;  // for custom colors
  uint16_t interval; // for flash interval
} __attribute__((packed));

// Response message structure
struct WarningResponse {
  uint8_t type;
  uint8_t status;  // 0: OK, 1: Error
  uint8_t data[6]; // Additional response data
} __attribute__((packed));

class WarningCommunication {
private:
  WarningPanel* panel;
  uint8_t buffer[32];
  uint8_t bufferIndex;
  unsigned long lastMessageTime;
  
  void processMessage(const WarningMessage& msg);
  void sendResponse(uint8_t msgType, uint8_t status, const uint8_t* data = nullptr, uint8_t dataLen = 0);
  bool validateMessage(const WarningMessage& msg);
  
public:
  WarningCommunication(WarningPanel* warningPanel);
  
  // Initialize communication
  void begin();
  
  // Process incoming data (call regularly in task)
  void update();
  
  // Send status update (called automatically or on request)
  void sendStatusUpdate();
  
  // Helper functions for message creation
  static WarningMessage createSetWarningMessage(WarningIcon icon, WarningState state, bool flashing = false, uint8_t brightness = 255);
  static WarningMessage createSetCustomWarningMessage(WarningIcon icon, uint32_t color, bool flashing = false, uint8_t brightness = 255);
  static WarningMessage createSetWorklightMessage(bool enabled, uint8_t startIndex, uint8_t endIndex, uint8_t brightness = 255, uint32_t color = 0, bool flashing = false);
  static WarningMessage createClearWarningMessage(WarningIcon icon);
  static WarningMessage createClearAllMessage();
  static WarningMessage createSetFlashIntervalMessage(uint16_t interval);
  static WarningMessage createGetStatusMessage();
};

#endif // WARNINGCOMMUNICATION_H
