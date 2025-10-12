# Reading Light Feature - Next Steps

## ✅ What's Complete

The reading light feature code is **100% implemented** and ready for hardware testing. All requirements from the issue have been addressed in software:

### Implemented Features
- ✅ Button handling code for SW6 (AUX 1 button)
- ✅ State machine: Off → White → Red → Off
- ✅ Hold-to-dim functionality
- ✅ Integration with existing LED system (LEDs 48-70)
- ✅ Safety features (auto-off when locked)
- ✅ HID compatibility maintained
- ✅ Comprehensive documentation

### Code Quality
- ✅ Follows existing code patterns
- ✅ Properly integrated with safety systems
- ✅ No breaking changes to existing functionality
- ✅ Well-documented and maintainable

## 🔧 Hardware Testing Required

Before closing the issue, you need to:

### 1. Build and Upload Firmware
```bash
cd "Ground Control Station/BottomPanelCode"
pio run -e PRODUCTION --target upload
```

### 2. Verify Basic Functionality
- [ ] Short press SW6: Light cycles Off → White → Red → Off
- [ ] Each state change should be visible
- [ ] Colors should be correct (White = RGB all on, Red = R only)

### 3. Test Dimming
- [ ] Turn light ON (white or red)
- [ ] Hold SW6 for more than 500ms
- [ ] Brightness should gradually decrease
- [ ] When reaching minimum, brightness wraps to maximum
- [ ] Release button to stop at desired brightness

### 4. Test Safety Features
- [ ] Turn light ON
- [ ] Lock system with key switch
- [ ] Light should turn OFF automatically
- [ ] Unlock system
- [ ] Light should remain OFF (doesn't auto-restore)

### 5. Test HID Integration
- [ ] Press SW6 with system unlocked
- [ ] Verify KEY_F19 is still sent (use HID monitor on Raspberry Pi)
- [ ] Verify payload arming still works (SW6 + SW8 combination)

### 6. Measure Current Draw
This is **critical** for safety:

**Equipment needed:**
- Multimeter (DC current mode, 10A range)
- Access to 5V power supply line

**Test procedure:**
```
1. Insert multimeter in series with 5V LED supply
2. With all LEDs off: Record idle current
3. Turn worklight to WHITE: Record peak current
4. Turn worklight to RED: Record current
5. Dim to various levels: Record current variation
```

**Expected values:**
- White mode (23 LEDs): ~1.4A maximum
- Red mode (23 LEDs): ~0.46A
- Dimmed: Proportionally lower

**Safety check:**
- [ ] Verify 5V supply can handle load
- [ ] Check for voltage drops during operation
- [ ] Ensure wiring is adequate for current
- [ ] Verify no overheating

### 7. Integration Testing
- [ ] Test with other switches active
- [ ] Verify no interference with warning panel LEDs
- [ ] Test with screen active
- [ ] Test during startup sequence
- [ ] Verify lock/unlock cycles

## 📝 Documentation to Read

1. **`Ground Control Station/BottomPanelCode/READING_LIGHT_FEATURE.md`**
   - Complete usage guide
   - Hardware specifications
   - Testing procedures
   - Troubleshooting

2. **`IMPLEMENTATION_SUMMARY.md`**
   - Technical implementation details
   - Code changes overview
   - Verification status

## 🐛 Known Limitations

These are **intentional design decisions**, not bugs:

1. **Dimming only works when light is ON**
   - Holding button when light is OFF does nothing
   - This prevents accidental dimming

2. **State changes on release**
   - Short press cycles state on button release, not press
   - This feels more natural and prevents accidental changes

3. **Brightness resets on state change**
   - Switching between White/Red resets brightness to 255
   - This ensures consistent starting point

4. **No smooth transitions**
   - State changes are instant
   - Could be added in future if needed

## 🔍 Troubleshooting

### Light doesn't turn on
1. Check system is unlocked (key switch)
2. Verify LEDs 48-70 are connected
3. Check 5V power supply
4. Review serial output for errors

### Dimming doesn't work
1. Ensure light is ON before holding
2. Hold for at least 500ms
3. Check that update() is called in main loop

### HID commands not working
1. Verify system is unlocked
2. Check USB connection to Raspberry Pi
3. Use HID debugging tools

### Light turns off unexpectedly
1. Check key switch position
2. Verify power supply stability

## ✨ Potential Future Enhancements

Ideas for improvement (not required now):
- Smooth fade transitions
- Configurable colors via serial commands
- Memory of last brightness level
- Adjustable hold threshold
- Auto-off timer
- Current limiting mode

## 🎯 Success Criteria

The feature is complete when:
- [x] Code compiles without errors
- [ ] Light responds to button presses correctly
- [ ] Dimming works as expected
- [ ] Safety features work (auto-off on lock)
- [ ] HID commands still function
- [ ] Current draw is within acceptable limits
- [ ] No interference with other systems
- [ ] User is satisfied with timing and brightness

## 📞 If You Need Changes

If after testing you find:
- Timing needs adjustment (hold threshold, dim speed)
- Brightness levels need tweaking
- Colors should be different
- Additional features needed

Just let me know! The code is designed to be easy to modify:
- Hold threshold: Line 115 in Switches.cpp (currently 500ms)
- Dim speed: Line 117 (currently 50ms interval)
- Brightness step: Line 122 (currently 5 units)
- Colors: Lines 296-300 in Switches.cpp

## 🎉 Closing the Issue

Once hardware testing is complete and successful:
1. Update the issue with test results
2. Mark remaining checklist items as complete
3. Close the issue as resolved
4. Merge this PR to main branch

---

**Questions or issues?** Tag me in the issue/PR and I'll help!
