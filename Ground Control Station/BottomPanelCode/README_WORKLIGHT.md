# Reading Light (Worklight) Feature

## Quick Start

The reading light is controlled by **SW6 (AUX 1 button)**:

- **Short press** (<500ms): Cycle through Off → White → Red → Off
- **Long hold** (>500ms): Dim the light (only when ON)

## Documentation

This PR includes comprehensive documentation:

### 1. [READING_LIGHT_FEATURE.md](READING_LIGHT_FEATURE.md)
**Start here!** Complete user guide covering:
- How to use the feature
- Button controls and behavior
- Safety features
- Troubleshooting

### 2. [WORKLIGHT_DIAGRAM.md](WORKLIGHT_DIAGRAM.md)
Technical diagrams showing:
- System architecture
- State machine flow
- LED layout
- Timing diagrams
- Data flow

### 3. [../../NEXT_STEPS.md](../../NEXT_STEPS.md)
Testing procedures:
- Hardware testing checklist
- Current draw measurements
- Integration testing
- Success criteria

### 4. [../../IMPLEMENTATION_SUMMARY.md](../../IMPLEMENTATION_SUMMARY.md)
Implementation details:
- What was implemented
- Code changes
- Technical specifications
- Verification status

## For Developers

### Modified Files
- `lib/Switches/Switches.h` - State tracking variables
- `lib/Switches/Switches.cpp` - Button handler logic

### Key Functions
```cpp
// In Switches namespace
void updateWorklight();  // Updates LED strip via WarningPanel

// Button handler for SW6
SwitchHandler::addSwitch(PINIO_SW6, callback);
```

### Configuration
All timing parameters in `lib/Switches/Switches.cpp`:
- Line 115: Hold threshold (500ms)
- Line 117: Dim update rate (50ms)
- Line 122: Brightness step (5 units)
- Lines 296-300: Colors (White/Red RGB)

### LEDs Used
- **LEDs 48-70** (23 LEDs) on the SK2812B strip
- Controlled via `WarningPanel::setWorklight()` API
- Thread-safe with FreeRTOS

### States
```cpp
enum WorklightState {
    WORKLIGHT_OFF = 0,
    WORKLIGHT_WHITE = 1,
    WORKLIGHT_RED = 2
};
```

## Testing Status

✅ Code complete and reviewed
✅ Documentation complete
⚠️ **Hardware testing required**

See [NEXT_STEPS.md](../../NEXT_STEPS.md) for testing procedures.

## Current Draw

Estimated (needs hardware verification):
- White mode: ~1.4A (23 LEDs × 60mA)
- Red mode: ~0.46A (23 LEDs × 20mA)
- Dimmed: Proportionally less

**Important:** Verify power supply can handle the load.

## Integration

The feature integrates seamlessly with existing systems:
- ✅ Uses WarningPanel API for LED control
- ✅ Works with lock/safety systems
- ✅ Maintains HID compatibility (KEY_F19)
- ✅ Thread-safe with FreeRTOS
- ✅ No breaking changes

## Questions?

- User guide: [READING_LIGHT_FEATURE.md](READING_LIGHT_FEATURE.md)
- Diagrams: [WORKLIGHT_DIAGRAM.md](WORKLIGHT_DIAGRAM.md)
- Testing: [NEXT_STEPS.md](../../NEXT_STEPS.md)
- Implementation: [IMPLEMENTATION_SUMMARY.md](../../IMPLEMENTATION_SUMMARY.md)

---

**Implementation by:** GitHub Copilot  
**Issue:** "Add reading light to Case"  
**Status:** Code complete, ready for hardware testing
