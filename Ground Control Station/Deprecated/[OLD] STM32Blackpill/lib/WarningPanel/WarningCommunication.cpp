#include "WarningCommunication.h"

WarningCommunication::WarningCommunication(WarningPanel* warningPanel) 
  : panel(warningPanel), bufferIndex(0), lastMessageTime(0) {
}

void WarningCommunication::begin() {
  bufferIndex = 0;
  lastMessageTime = millis();
}

void WarningCommunication::update() {
  // Check for timeout (reset buffer if no data for 1 second)
  if (millis() - lastMessageTime > 1000 && bufferIndex > 0) {
    bufferIndex = 0;
  }
  
  // Process incoming serial data
  while (Serial.available() && bufferIndex < sizeof(buffer)) {
    buffer[bufferIndex++] = Serial.read();
    lastMessageTime = millis();
    
    // Check if we have a complete message
    if (bufferIndex >= sizeof(WarningMessage)) {
      WarningMessage* msg = reinterpret_cast<WarningMessage*>(buffer);
      
      if (validateMessage(*msg)) {
        processMessage(*msg);
      } else {
        sendResponse(msg->type, 1); // Error response
      }
      
      // Reset buffer
      bufferIndex = 0;
    }
  }
}

void WarningCommunication::processMessage(const WarningMessage& msg) {
  if (panel == nullptr) {
    sendResponse(msg.type, 1); // Error: no panel
    return;
  }
  
  switch (msg.type) {
    case MSG_SET_WARNING:
      panel->setWarning(
        (WarningIcon)msg.icon, 
        (WarningState)msg.state, 
        (msg.flags & 0x01) != 0, 
        msg.brightness
      );
      sendResponse(msg.type, 0); // OK
      break;
      
    case MSG_SET_WARNING_CUSTOM:
      panel->setWarningCustom(
        (WarningIcon)msg.icon, 
        msg.color, 
        (msg.flags & 0x01) != 0, 
        msg.brightness
      );
      sendResponse(msg.type, 0); // OK
      break;
      
    case MSG_SET_WORKLIGHT:
      panel->setWorklight(
        (msg.flags & 0x01) != 0, // enabled
        msg.startIndex,
        msg.endIndex,
        msg.brightness, 
        msg.color, 
        (msg.flags & 0x02) != 0  // flashing
      );
      sendResponse(msg.type, 0); // OK
      break;
      
    case MSG_CLEAR_WARNING:
      panel->clearWarning((WarningIcon)msg.icon);
      sendResponse(msg.type, 0); // OK
      break;
      
    case MSG_CLEAR_ALL:
      panel->clearAllWarnings();
      panel->setWorklightEnabled(false);
      sendResponse(msg.type, 0); // OK
      break;
      
    case MSG_SET_FLASH_INTERVAL:
      panel->setFlashInterval(msg.interval);
      sendResponse(msg.type, 0); // OK
      break;
      
    case MSG_GET_STATUS:
      sendStatusUpdate();
      break;
      
    case MSG_PING:
      sendResponse(msg.type, 0); // Pong
      break;
      
    default:
      sendResponse(msg.type, 1); // Unknown message type
      break;
  }
}

void WarningCommunication::sendResponse(uint8_t msgType, uint8_t status, const uint8_t* data, uint8_t dataLen) {
  WarningResponse response;
  response.type = msgType;
  response.status = status;
  
  // Copy data if provided
  if (data != nullptr && dataLen <= sizeof(response.data)) {
    memcpy(response.data, data, dataLen);
  } else {
    memset(response.data, 0, sizeof(response.data));
  }
  
  // Send response over serial
  Serial.write(reinterpret_cast<uint8_t*>(&response), sizeof(response));
}

void WarningCommunication::sendStatusUpdate() {
  if (panel == nullptr) return;
  
  // Send warning states
  for (int i = 0; i < NUM_WARNING_LEDS; i++) {
    uint8_t statusData[6];
    statusData[0] = i; // icon index
    statusData[1] = (uint8_t)panel->getWarningState((WarningIcon)i);
    statusData[2] = panel->isWarningFlashing((WarningIcon)i) ? 1 : 0;
    statusData[3] = panel->getWarningBrightness((WarningIcon)i);
    statusData[4] = 0; // reserved
    statusData[5] = 0; // reserved
    
    sendResponse(MSG_GET_STATUS, 0, statusData, sizeof(statusData));
  }
  
  // Send worklight status
  uint8_t worklightData[6];
  worklightData[0] = 0xFF; // Indicates worklight status
  worklightData[1] = panel->isWorklightEnabled() ? 1 : 0;
  worklightData[2] = panel->isWorklightFlashing() ? 1 : 0;
  worklightData[3] = panel->getWorklightBrightness();
  uint32_t color = panel->getWorklightColor();
  worklightData[4] = (color >> 16) & 0xFF; // R
  worklightData[5] = (color >> 8) & 0xFF;  // G
  // Note: B component would need another message or larger data field
  
  sendResponse(MSG_GET_STATUS, 0, worklightData, sizeof(worklightData));
}

bool WarningCommunication::validateMessage(const WarningMessage& msg) {
  // Basic validation
  switch (msg.type) {
    case MSG_SET_WARNING:
    case MSG_SET_WARNING_CUSTOM:
    case MSG_CLEAR_WARNING:
      return msg.icon < NUM_WARNING_LEDS && msg.state <= WARNING_INFO;
      
    case MSG_SET_WORKLIGHT:
    case MSG_CLEAR_ALL:
    case MSG_SET_FLASH_INTERVAL:
    case MSG_GET_STATUS:
    case MSG_PING:
      return true;
      
    default:
      return false;
  }
}

// Helper functions for message creation
WarningMessage WarningCommunication::createSetWarningMessage(WarningIcon icon, WarningState state, bool flashing, uint8_t brightness) {
  WarningMessage msg = {0};
  msg.type = MSG_SET_WARNING;
  msg.icon = icon;
  msg.state = state;
  msg.flags = flashing ? 0x01 : 0x00;
  msg.brightness = brightness;
  return msg;
}

WarningMessage WarningCommunication::createSetCustomWarningMessage(WarningIcon icon, uint32_t color, bool flashing, uint8_t brightness) {
  WarningMessage msg = {0};
  msg.type = MSG_SET_WARNING_CUSTOM;
  msg.icon = icon;
  msg.flags = flashing ? 0x01 : 0x00;
  msg.brightness = brightness;
  msg.color = color;
  return msg;
}

WarningMessage WarningCommunication::createSetWorklightMessage(bool enabled, uint8_t startIndex, uint8_t endIndex, uint8_t brightness, uint32_t color, bool flashing) {
  WarningMessage msg = {0};
  msg.type = MSG_SET_WORKLIGHT;
  msg.startIndex = startIndex;
  msg.endIndex = endIndex;
  msg.flags = (enabled ? 0x01 : 0x00) | (flashing ? 0x02 : 0x00);
  msg.brightness = brightness;
  msg.color = color;
  return msg;
}

WarningMessage WarningCommunication::createClearWarningMessage(WarningIcon icon) {
  WarningMessage msg = {0};
  msg.type = MSG_CLEAR_WARNING;
  msg.icon = icon;
  return msg;
}

WarningMessage WarningCommunication::createClearAllMessage() {
  WarningMessage msg = {0};
  msg.type = MSG_CLEAR_ALL;
  return msg;
}

WarningMessage WarningCommunication::createSetFlashIntervalMessage(uint16_t interval) {
  WarningMessage msg = {0};
  msg.type = MSG_SET_FLASH_INTERVAL;
  msg.interval = interval;
  return msg;
}

WarningMessage WarningCommunication::createGetStatusMessage() {
  WarningMessage msg = {0};
  msg.type = MSG_GET_STATUS;
  return msg;
}
