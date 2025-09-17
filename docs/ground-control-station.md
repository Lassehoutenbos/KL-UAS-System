---
layout: default
title: Ground Control Station
---

# Ground Control Station

The KL UAS ground control station (GCS) provides the operator with physical controls, telemetry, and situational awareness tailored for drone missions.

## System architecture

- **Control panels** – Custom-designed top and bottom panels with tactile switches, rotary encoders, and lighting for mission workflows.
- **Display system** – Integrated monitors report flight status and mission feedback in real time.
- **Communication** – Interfaces with ground control software and telemetry radios to keep the operator synced with the aircraft.
- **Power management** – Independent power supplies with battery backup to keep the station online in the field.
- **Real-time ADS-B monitoring** – Alerts the crew about nearby aircraft for deconfliction and safety.

## Bottom panel electronics

| Subsystem | Components |
| --- | --- |
| Main controller | STM32F411CE (BlackPill) running FreeRTOS |
| I/O expansion | MCP23017 16-bit expander for switches and LEDs |
| Lighting control | PCA9685 16-channel PWM driver for Neopixel strips |
| Visual feedback | 1.44" ST7735 TFT display (128×128) for power status |
| Inputs | Mission switches and buttons mapped to USB HID events |
| Status monitoring | RGB LEDs, temperature sensing, and fan control |

The bottom panel hardware is designed around modular PlatformIO firmware, making it straightforward to adapt controls or add new mission behaviors.

## Additional resources

- [Button mapping reference](https://github.com/{{ site.github.repository_nwo }}/blob/main/Ground%20Control%20Station/GCS_Button_Mapping.md)
- [Logs and telemetry captures](https://github.com/{{ site.github.repository_nwo }}/tree/main/Docs/Logs)
- [Code documentation](https://github.com/{{ site.github.repository_nwo }}/blob/main/Docs/CodeDocumentation.md)

Return to the [documentation home](index.html).
