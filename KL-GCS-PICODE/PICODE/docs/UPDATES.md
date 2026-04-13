# GCS To-Do & Updates Tracker

## UI / UX Enhancements

- [x] **Quick Panel**: Segmented layout implemented for lighting, quick layouts, and system stats.
- [x] **Quick Layouts**: Added mode chips with selectable presets.
  Modes: PILOT (flight focus), MAP (map focus), DEBUG (system focus), POWER (power/thermal focus), CLEAN (minimal).
- [x] **Quick Panel Text**: Removed abbreviations for labels and system stats where possible.

```text
┌────────────────────────────────────────────┐
│ QUICK PANEL                         [ ✕ ]  │
├────────────────────────────────────────────┤
│ LIGHTING                                   │
│ WORKLIGHT ●   AMBIENT ●   COLOR ● ● ● ● ● │
│ BRIGHTNESS ─────────────●────────── 72%    │
│ SCREEN L 72  SCREEN R 70  LED STRIP 80     │
├────────────────────────────────────────────┤
│ QUICK LAYOUTS                              │
│ [PILOT] [MAP] [DEBUG] [POWER] [CLEAN]      │
├────────────────────────────────────────────┤
│ PROCESSOR 54°   MEMORY 63%   DISK 41%      │
└────────────────────────────────────────────┘
```

- [x] **Case SVG**: Improved styling and added to the **Dash Page** overview area.
- [x] **Case Settings Page**: Sliders grouped into clearer card rows with improved scale readability.
- [x] **Slider Alignment**: Aligned slider label/value columns across brightness rows.

## Mission Control

- [x] **Flight Modes**: Added mode profile section with per-mode settings and defaults for newly added waypoints.
  - Updated to actual mission modes: WAYPOINT, LOITER, CIRCLE, SPLINE, LAND.

## Hardware & Peripherals

- [x] **Worklight LEDs**: Improved lighting controls and live LED/brightness indication in the Quick Panel.
