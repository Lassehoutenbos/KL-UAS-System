# Reading Light (Worklight) - Architecture Diagram

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         Ground Control Station                    │
│                            Bottom Panel                           │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  Hardware Components                                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────┐         ┌─────────────┐        ┌──────────────┐  │
│  │  SW6     │────────▶│  STM32F411  │───────▶│  SK2812B     │  │
│  │ (Button) │ PINIO_6 │  BlackPill  │  PB3   │  LED Strip   │  │
│  └──────────┘         └─────────────┘        └──────────────┘  │
│   AUX 1 Button         Microcontroller        71 LEDs total     │
│                                                LEDs 48-70 used   │
│                                                (23 LEDs)         │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  Software Architecture                                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                      main.cpp (FreeRTOS)                    │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │ │
│  │  │   uiTask     │  │  tempTask    │  │ warningTask  │    │ │
│  │  │              │  │              │  │              │    │ │
│  │  │  Switches::  │  │              │  │WarningPanel::│    │ │
│  │  │   update()   │  │              │  │   update()   │    │ │
│  │  └──────┬───────┘  └──────────────┘  └──────┬───────┘    │ │
│  └─────────┼───────────────────────────────────┼─────────────┘ │
│            │                                     │               │
│            ▼                                     ▼               │
│  ┌─────────────────────────────┐  ┌───────────────────────────┐│
│  │   Switches Namespace        │  │   WarningPanel Class      ││
│  │  ┌─────────────────────┐   │  │  ┌────────────────────┐  ││
│  │  │ SW6 Button Handler  │   │  │  │  setWorklight()    │  ││
│  │  │                     │   │  │  │                    │  ││
│  │  │ • Press Detection   │───┼──┼─▶│  • LED Control    │  ││
│  │  │ • State Machine     │   │  │  │  • Brightness     │  ││
│  │  │ • Dimming Logic     │   │  │  │  • Color Setting  │  ││
│  │  │                     │   │  │  │  • Thread-safe    │  ││
│  │  └─────────────────────┘   │  │  └────────────────────┘  ││
│  │                             │  │           │              ││
│  │  updateWorklight()          │  │           │              ││
│  └─────────────────────────────┘  └───────────┼──────────────┘│
│                                                 │               │
│                                                 ▼               │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │              Adafruit_NeoPixel Library                   │  │
│  │  ┌───────────────────────────────────────────────────┐  │  │
│  │  │  strip.setPixelColor(48-70, r, g, b, w)          │  │  │
│  │  │  strip.show()                                     │  │  │
│  │  └───────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Button Handler State Machine

```
┌──────────────────────────────────────────────────────────────────┐
│                    SW6 Button Press Flow                          │
└──────────────────────────────────────────────────────────────────┘

                    ┌─────────────┐
                    │   BUTTON    │
                    │   PRESSED   │
                    └──────┬──────┘
                           │
                    Record timestamp
                    sw6WasPressed = true
                           │
                           ▼
            ┌──────────────────────────────┐
            │  Wait for release or hold    │
            └──────────┬───────────────────┘
                       │
        ┌──────────────┴──────────────┐
        │                             │
        │ Duration < 500ms            │ Duration > 500ms
        │ (Short Press)               │ (Hold)
        │                             │
        ▼                             ▼
  ┌───────────┐              ┌────────────────┐
  │  BUTTON   │              │  DIMMING MODE  │
  │ RELEASED  │              │                │
  └─────┬─────┘              │ Every 50ms:    │
        │                    │ brightness -= 5│
        │                    │ if < 10: = 255 │
        ▼                    │                │
  ┌───────────┐              │ updateWorklight()│
  │ State     │              └────────┬───────┘
  │ Machine   │                       │
  └─────┬─────┘                       │
        │                             │
        ▼                             ▼
  ┌─────────────┐           ┌─────────────────┐
  │ OFF → WHITE │           │  Wait for       │
  │ WHITE → RED │           │  button release │
  │ RED → OFF   │           └─────────────────┘
  └──────┬──────┘
         │
         │ brightness = 255
         │
         ▼
  ┌──────────────┐
  │updateWorklight()│
  └──────────────┘
```

## State Machine Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│                 Worklight State Transitions                         │
└────────────────────────────────────────────────────────────────────┘

                         Short Press
             ┌───────────────────────────────────┐
             │                                   │
             │                                   │
        ┌────▼────┐  Short Press   ┌────────┐  │
        │   OFF   │───────────────▶│ WHITE  │  │
        │         │                │ (255)  │  │
        └────▲────┘                └───┬────┘  │
             │                         │       │
             │                         │       │
             │     Short Press         │       │
             │         ┌───────────────┘       │
             │         │                       │
             │    ┌────▼────┐                 │
             └────│   RED   │─────────────────┘
                  │  (255)  │   Short Press
                  └────┬────┘
                       │
                       │  Hold > 500ms
                       │
                  ┌────▼────────────┐
                  │   DIMMING       │
                  │   255 → 10      │
                  │   wraps to 255  │
                  └─────────────────┘
```

## LED Strip Layout

```
┌────────────────────────────────────────────────────────────────────┐
│              SK2812B LED Strip (71 LEDs Total)                     │
└────────────────────────────────────────────────────────────────────┘

LED Index:
0                                                                    70
│◄─────────────────────────────────────────────────────────────────►│

┌─────────────┬──────────────┬────────────────────────────────┐
│  Switch     │   Warning    │        Worklight              │
│  LEDs       │   Panel      │      (Reading Light)          │
│  0-37       │   38-47      │        48-70                  │
│  (38 LEDs)  │  (10 LEDs)   │       (23 LEDs)               │
└─────────────┴──────────────┴────────────────────────────────┘

     │              │                     │
     │              │                     │
     ▼              ▼                     ▼
  Switches      Warning Icons       Reading Light
  Feedback      Temperature         (User controlled
  (auto)        GPS, etc.           via SW6 button)
                (system controlled)
```

## Timing Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│                  Button Press Timing                                │
└────────────────────────────────────────────────────────────────────┘

Time (ms):  0         500        1000       1500       2000
            │          │          │          │          │
            │          │          │          │          │
SHORT      ┌┐                                           
PRESS:     │└──────────                                 
            │◄────►│                                     
            │ <500ms │                                    
            │        │                                    
            │ State Change                                
            │                                             
                                                          
HOLD &     ┌──────────────────────────────────┐         
DIM:       │                                  └─────    
            │◄─────►│                                     
            │ 500ms │                                     
            │       │◄─►│◄─►│◄─►│◄─►│◄─►│              
            │       │50ms intervals                      
            │       │                                     
            │       │ Dimming updates every 50ms         
            │                                             
            │                                             
Brightness: 255     255  250 245 240 235 230 ...        
                          ▼   ▼   ▼   ▼   ▼             
```

## Data Flow

```
┌────────────────────────────────────────────────────────────────────┐
│                     Data Flow Diagram                               │
└────────────────────────────────────────────────────────────────────┘

   User Input                                      LED Output
       │                                               ▲
       │                                               │
       ▼                                               │
  ┌─────────┐                                    ┌────────────┐
  │  SW6    │                                    │  LEDs      │
  │ Button  │                                    │  48-70     │
  └────┬────┘                                    └─────▲──────┘
       │                                               │
       │ GPIO Read (PINIO_SW6)                        │ SPI Data (PB3)
       │                                               │
       ▼                                               │
  ┌──────────────────┐                          ┌─────┴────────┐
  │ SwitchHandler    │                          │ NeoPixel     │
  │ (Polling)        │                          │ Library      │
  └────┬─────────────┘                          └─────▲────────┘
       │                                               │
       │ State Change                                  │
       │                                               │
       ▼                                               │
  ┌──────────────────┐        ┌──────────────┐       │
  │ SW6 Lambda       │───────▶│ updateWork-  │───────┤
  │ Callback         │        │ light()      │       │
  └──────┬───────────┘        └──────────────┘       │
       │                             │                │
       │ Time Tracking               │ State Info     │
       │ State Machine               │                │
       │                             ▼                │
       │                      ┌──────────────┐       │
       │                      │WarningPanel  │───────┘
       │                      │setWorklight()│
       │                      └──────────────┘
       │
       │ HID Output
       ▼
  ┌──────────────────┐
  │ BootKeyboard     │
  │ KEY_F19          │
  └──────────────────┘
       │
       │ USB
       ▼
  Raspberry Pi
```

## Thread Safety

```
┌────────────────────────────────────────────────────────────────────┐
│                   FreeRTOS Task Interaction                         │
└────────────────────────────────────────────────────────────────────┘

  uiTask (Priority 2)              warningTask (Priority 1)
  ───────────────────              ────────────────────────
       │                                    │
       │ Switches::update()                │
       │      │                             │
       │      ▼                             │
       │  SW6 Handler                       │
       │      │                             │
       │      │ updateWorklight()           │
       │      └────────────┐                │
       │                   │                │
       │                   ▼                ▼
       │          ┌────────────────────────────────┐
       │          │   WarningPanel (Thread-safe)   │
       │          │   ┌──────────────────┐         │
       │          │   │  Mutex Protected  │         │
       │          │   │  • worklightConfig│         │
       │          │   │  • updateRequired │         │
       │          │   └──────────────────┘         │
       │          └────────────────────────────────┘
       │                   │                │
       │                   │                │
       │                   │                │ WarningPanel::update()
       │                   │                │
       │                   │                ▼
       │                   │          Apply LED changes
       │                   │          strip.show()
       │                   │                │
       │                   └────────────────┘
       │
       ▼
  (continues UI loop)
```

## Memory Usage

```
┌────────────────────────────────────────────────────────────────────┐
│                       Memory Footprint                              │
└────────────────────────────────────────────────────────────────────┘

Static Variables (Switches namespace):
├── worklightState          1 byte  (enum)
├── sw6PressStartTime       4 bytes (unsigned long)
├── sw6WasPressed           1 byte  (bool)
├── worklightBrightness     1 byte  (uint8_t)
└── lastDimUpdate           4 bytes (unsigned long)
                          ────────
                           11 bytes total

WarningPanel Class:
└── worklightConfig         9 bytes (struct)
    ├── enabled             1 byte
    ├── brightness          1 byte
    ├── color               4 bytes
    ├── flashing            1 byte
    ├── startIndex          1 byte
    └── endIndex            1 byte

LED Strip Buffer (shared):
└── 71 LEDs × 4 bytes      284 bytes (R,G,B,W)

Total Additional:           ~20 bytes
```

---

This diagram provides a visual reference for understanding how the reading light
feature integrates with the existing system architecture.
