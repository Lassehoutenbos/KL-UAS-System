# Reading Light Feature Documentation

## Overview

The reading light feature provides illumination using the last 23 LEDs (48-70) of the SK2812B LED strip. The light is controlled by the **SW6 button (AUX 1)** and supports multiple modes and brightness adjustment.

## Hardware Configuration

- **LED Range**: LEDs 48-70 (23 LEDs total)
- **LED Type**: SK2812B RGB strip (5V)
- **Control Pin**: Connected to the same data line as other LEDs (PB3)
- **Button**: SW6 (PINIO_SW6 on IO Expander)

## Button Control

### Single Press (< 500ms)
Cycles through three states:
1. **OFF** → **WHITE** (full brightness, 255)
2. **WHITE** → **RED** (full brightness, 255)
3. **RED** → **OFF**

### Long Press (> 500ms hold)
When the light is ON (White or Red state):
- **Hold** the button to activate dimming mode
- Brightness decreases from 255 to 10 in steps of 5
- Updates every 50ms while held
- When brightness reaches minimum (10), it wraps back to full brightness (255)
- Release button to stop dimming at current brightness

### Important Notes
- Dimming only works when light is already ON (not in OFF state)
- Short presses reset brightness to 255
- The light state persists until changed by button press or system lock

## Safety Features

### System Lock Integration
- When the key switch is turned to LOCK position, the reading light automatically turns OFF
- The light state is preserved when unlocking (remains OFF until manually turned on)

### HID Compatibility
- SW6 maintains its original function: sends KEY_F19 for payload arming
- Both reading light control and HID command work simultaneously
- The `armPayload1` state is still updated for SW8 safety interlock

## Implementation Details

### State Machine
```cpp
enum WorklightState {
    WORKLIGHT_OFF = 0,
    WORKLIGHT_WHITE = 1,
    WORKLIGHT_RED = 2
};
```

### Color Configuration
- **White Mode**: RGB(255, 255, 255)
- **Red Mode**: RGB(255, 0, 0)
- **OFF**: All LEDs off

### Timing Parameters
- **Short Press Threshold**: 500ms
- **Dimming Update Interval**: 50ms
- **Brightness Step**: 5 units per update
- **Brightness Range**: 10 to 255

## Code Location

- **Header**: `lib/Switches/Switches.h`
- **Implementation**: `lib/Switches/Switches.cpp`
- **LED Control**: Uses `WarningPanel::setWorklight()` API

## Testing Procedure

### Manual Testing (requires hardware)

1. **Power up the system** and unlock with key switch
2. **Test state cycling**:
   - Press SW6 briefly → Light should turn ON (WHITE)
   - Press SW6 briefly → Light should change to RED
   - Press SW6 briefly → Light should turn OFF
3. **Test dimming**:
   - Press SW6 briefly to turn light ON (WHITE)
   - Hold SW6 for > 500ms → Brightness should start decreasing
   - Continue holding → Brightness wraps to full when reaching minimum
   - Release SW6 → Brightness stays at current level
4. **Test brightness reset**:
   - While light is dimmed, press SW6 briefly
   - Light should change state AND reset to full brightness
5. **Test lock behavior**:
   - Turn light ON
   - Lock the system with key switch
   - Light should turn OFF immediately
6. **Test HID compatibility**:
   - Ensure KEY_F19 is still sent when SW6 is pressed (check with HID monitor)

### Expected Behavior
- Smooth brightness transitions during dimming
- Immediate response to button presses
- No LED flickering
- State changes only on button release for short presses
- Continuous dimming while button held

## Current Draw Considerations

The issue mentions checking if 144/m LED strip is usable regarding current draw:

- **23 LEDs** at full white (255, 255, 255) at maximum
- Estimated: 23 LEDs × 60mA = ~1.4A peak (worst case)
- Red mode uses less: 23 LEDs × 20mA = ~0.46A
- Dimming reduces current proportionally

**Recommendation**: Verify the 5V power supply can handle the additional load, especially when other LEDs are active.

## Future Enhancements

Possible improvements (not implemented):
- [ ] Smooth brightness transitions (fade in/out)
- [ ] Configurable colors beyond White/Red
- [ ] Memory of last brightness level
- [ ] Auto-off timer
- [ ] Integration with ambient light sensor

## Troubleshooting

### Light doesn't turn on
- Check system is unlocked (key switch in ON position)
- Verify LEDs 48-70 are properly connected
- Ensure 5V power supply is connected and adequate
- Check `WarningPanel` is properly initialized in `main.cpp`

### Dimming doesn't work
- Ensure light is already ON before holding button
- Hold button for at least 500ms
- Check that `update()` is called regularly in main loop

### Light turns off unexpectedly
- System may have been locked - check key switch
- Verify power supply stability

### HID commands not working
- This feature maintains backward compatibility
- KEY_F19 should still be sent - use HID debugging tools to verify
- Check that system is unlocked and confirmed

## Related Files

- `lib/WarningPanel/WarningPanel.h` - Worklight API
- `lib/WarningPanel/WarningPanel.cpp` - LED control implementation
- `lib/leds/leds.h` - LED strip configuration
- `Ground Control Station/GCS_Button_Mapping.md` - Button assignments
