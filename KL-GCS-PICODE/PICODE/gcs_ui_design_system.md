# GCS UI Color System

## 1. Core Principle
Color is used only to communicate **state, hierarchy, and priority**.
No decorative or unnecessary color usage.

---

## 2. Base Colors (Background)

- Primary Background: #0b0d10
- Secondary Background: #12161b
- Elevated Panel: #1a2027

Rules:
- Always use dark backgrounds
- Create depth using subtle contrast between layers
- Avoid pure black (#000000) for main surfaces

---

## 3. Text Colors

- Primary Text: #e8edf2
- Secondary Text: #8f9baa
- Disabled Text: #5c6673

Rules:
- High contrast for critical information
- Secondary text only for labels, never for values

---

## 4. Accent Colors

### Primary Accent (Action / Active)
- #e3d049 (industrial yellow)

Usage:
- Active buttons
- Selected elements
- Highlights

### Secondary Accent (Telemetry / Neutral Data)
- #7fd6ff (soft blue)

Usage:
- Data visualization
- Graphs
- Neutral indicators

Rules:
- Never mix accents in the same component
- Use sparingly to avoid visual noise

---

## 5. Status Colors

### OK / Connected
- #62d48f (green)

### Warning
- #ffb347 (orange)

### Error / Critical
- #ff5b5b (red)

Usage:
- System state
- Hardware status
- Alerts

Rules:
- Always pair color with text or icon
- Critical states must override all other colors

---

## 6. Interactive States

### Idle
- Background: #1a2027
- Border: #2a313a

### Hover / Touch
- Slight brightness increase
- Subtle glow using accent color

### Active
- Use primary accent (#e3d049)

### Critical Active (e.g. ARM)
- Use red (#ff5b5b)

---

## 7. Borders & Lines

- Default Border: #2a313a
- Soft Divider: rgba(255,255,255,0.05)

Rules:
- Use borders to define structure
- Avoid heavy separators

---

## 8. Overlays & Transparency

- Panel Overlay: rgba(255,255,255,0.02 – 0.05)
- HUD Overlay: rgba(0,0,0,0.2 – 0.4)

Rules:
- Maintain readability over video
- Do not reduce contrast of critical data

---

## 9. Priority Rules

1. Red (critical)
2. Orange (warning)
3. Yellow (active/action)
4. Blue (data)
5. White/Grey (default)

Rules:
- Higher priority colors must dominate visually
- Never compete multiple high-priority colors in one area

---

## 10. Do Not

- Do not use bright/neon colors randomly
- Do not use gradients unless extremely subtle
- Do not use multiple accents in one component
- Do not reduce contrast for aesthetic reasons

---

## 11. Summary

Color = function.

Every color must answer:
- What is the state?
- Is it critical?
- Does the user need to react?

If not, remove it.

