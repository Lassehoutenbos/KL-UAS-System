# Drone

This repository contains design files, documentation and firmware for an
educational unmanned aircraft system.  Everything here is provided without
any warranty and should only be used in compliance with your local
regulations.

## Description

This is a custom-built quadcopter featuring a SpeedyBee F7 V3 flight controller stack with INAV firmware. The drone is equipped with 10-inch folding propellers powered by four Emax GT2218 1100KV brushless motors, providing flight stability and endurance. With its powerful motor configuration, the aircraft can lift a considerable amount of weight, making it suitable for payload missions and extended flight operations. It includes advanced features such as optical flow and LiDAR sensors for positioning, GPS with compass for navigation, and an integrated FPV system with Walksnail Avatar. The aircraft is designed for autonomous flight systems, mission planning, and drone operations.

## Parts list

- [4x Emax GT2218 1100KV brushless motor](https://emaxmodel.com/products/gt2218)
- [2x clockwise Gemfan F1051-3 Folding](https://www.rotorama.com/product/gemfan-f1051-folding-3)
- [2x counter-clockwise Gemfan F1051-3 Folding](https://www.rotorama.com/product/gemfan-f1051-folding-3)
- [1x SpeedyBee F7 V3 stack](https://www.speedybee.com/speedybee-f7-v3-bl32-50a-30x30-stack/)
- [1x Mateksys Optical Flow & Lidar Sensor 3901-L0X](https://www.mateksys.com/?portfolio=3901-l0x)
- [1x TRC Car Lipo TRC Car Lipo 50c 11,1 volt 5400mah](https://www.toprc.nl/trc-car-lipo-50c-3s-5400mah-xt90-stekker.html)
- [1x Foxeer M10Q 250 GPS module with compass](https://www.rotorama.com/product/foxeer-m10q-250-gps-modul-s-kompasem)
- [1x RadioMaster RP4TD-M ELRS RX-TX](https://droneshop.nl/radiomaster-rp4td-m-expresslrs-2-4ghz-race-receiver)
- [1x RadioMaster Pocket ELRS](https://droneshop.nl/radiomaster-pocket-elrs)
- [4x Neopixel 8X WS2812B](https://www.benselectronics.nl/neopixel-flora-rgb-ws2812-adresseerbare-led.html)
- [1x Walksnail VRX](https://droneshop.nl/walksnail-vrx-fpv)
- [1x Walksnail Avatar HD Nano Kit V3](https://droneshop.nl/walksnail-avatar-hd-nano-v3)
- [1x Body](https://www.hobbydrone.cz/en/frame-pilotix-mark4-10-partizan-edition-10-/)

## Ground Control Station

To control the drone during these operations, the system includes a custom-built ground control station that provides comprehensive mission control capabilities.

### GCS System Architecture

The Ground Control Station consists of multiple components working together:

- **Control Panels**: Custom-designed top and bottom panels with physical controls
- **Display System**: Integrated screens for status monitoring and mission feedback
- **Communication**: Interfaces with ground control software and drone telemetry
- **Power Management**: Independent power systems with battery backup
- **Real Time ADSB Monitoring**: Monitors ADSB frequencies to warn about ther aircraft

### Bottom Panel Specifications

The bottom panel contains the main electronics for the control interface:

- **Main Controller**: STM32F411CE (BlackPill board)
- **I/O Expansion**: MCP23017 16-bit I/O expander for switches and LEDs
- **PWM Controller**: PCA9685 16-channel PWM driver for LED control
- **Display**: 1.44" ST7735 TFT display (128x128 pixels) for power status
- **Input Controls**: Multiple switches and buttons for mission control
- **Status Indicators**: RGB LEDs for system status and alerts
- **Connectivity**: USB HID interface for computer integration
- **Power Management**: Battery voltage monitoring with external power input
- **Temperature Sensors**: Integrated thermal monitoring with fan control
- **Real-time OS**: FreeRTOS for reliable operation
- **Programming**: PlatformIO-based development with modular firmware architecture

The ground control station features a custom PCB design with dedicated controls for drone operations, mission planning, and real-time telemetry monitoring.

[Button descriptions](Ground Control Station/GCS_Button_Mapping.md)

### Disclaimer

The information in this repository is provided for educational use only.
Always operate your drone responsibly and adhere to local laws and
regulations.
