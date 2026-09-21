---
version: alpha
name: Zrythm
description: Dark-first design system for the Zrythm DAW (Qt Quick/QML)
colors:
  primary: "#FFAE00"
  secondary: "#323232"
  surface: "#161616"
  background: "#000000"
  on-surface: "#E3E3E3"
  alternate-surface: "#0F0F0F"
  error: "#D90368"
  solo: "#009B86"
  superorange: "#FF5500"
  spring-green: "#40FFA0"
  jonquil-yellow: "#FFD100"
  munsell-red: "#FF0040"
  electric-purple: "#A654F7"
  gunmetal: "#2E3138"
  placeholder: "#FFFFFF80"
  shadow: "#000000B3"
  overlay-standout: "#FFFFFF40"
  overlay-hover: "#FFFFFF1A"
typography:
  button:
    fontFamily: Noto Sans
    fontSize: 12px
    fontWeight: 700
  semibold:
    fontFamily: Noto Sans
    fontSize: 12px
    fontWeight: 500
  body:
    fontFamily: Noto Sans
    fontSize: 12px
    fontWeight: 400
  track-name:
    fontFamily: Noto Sans
    fontSize: 12px
    fontWeight: 500
  faded:
    fontFamily: Noto Sans
    fontSize: 11px
    fontWeight: 400
  arranger:
    fontFamily: Noto Sans
    fontSize: 11px
    fontWeight: 500
  arranger-bold:
    fontFamily: Noto Sans
    fontSize: 11px
    fontWeight: 700
  small:
    fontFamily: Noto Sans
    fontSize: 10px
    fontWeight: 400
  x-small:
    fontFamily: Noto Sans
    fontSize: 9px
    fontWeight: 400
  xx-small:
    fontFamily: Noto Sans
    fontSize: 8px
    fontWeight: 400
rounded:
  sm: 4px
  md: 6px
  lg: 9px
  full: 12px
spacing:
  base: 4px
  control-height: 24px
  dropdown-icon-size: 16px
components:
  button:
    backgroundColor: "{colors.secondary}"
    textColor: "{colors.on-surface}"
    typography: "{typography.button}"
    rounded: "{rounded.lg}"
    height: 24px
    padding: 6px
  button-hover:
    backgroundColor: "#414141"
  button-pressed:
    backgroundColor: "#262626"
  button-toggled:
    backgroundColor: "{colors.primary}"
    textColor: "{colors.surface}"
  button-emphasized:
    backgroundColor: "{colors.on-surface}"
    textColor: "{colors.surface}"
  button-destructive:
    backgroundColor: "{colors.error}"
    textColor: "#FFFFFF"
  combo:
    backgroundColor: "{colors.secondary}"
    textColor: "{colors.on-surface}"
    typography: "{typography.button}"
    rounded: "{rounded.lg}"
    height: 24px
    width: 140px
  input:
    backgroundColor: "{colors.secondary}"
    textColor: "{colors.on-surface}"
    typography: "{typography.body}"
    rounded: "{rounded.sm}"
    height: 24px
    width: 200px
    padding: 4px
  checkbox:
    backgroundColor: "{colors.secondary}"
    rounded: "{rounded.sm}"
    size: 16px
  checkbox-checked:
    backgroundColor: "{colors.primary}"
  tab:
    backgroundColor: "{colors.secondary}"
    textColor: "{colors.on-surface}"
    typography: "{typography.semibold}"
    rounded: "{rounded.full}"
    height: 24px
  tab-selected:
    backgroundColor: "{colors.primary}"
    textColor: "{colors.surface}"
  menu-item:
    textColor: "{colors.on-surface}"
    typography: "{typography.semibold}"
    rounded: "{rounded.sm}"
    height: 24px
    padding: 8px
  menu-item-highlighted:
    backgroundColor: "{colors.primary}"
    textColor: "{colors.surface}"
  list-item:
    textColor: "{colors.on-surface}"
    typography: "{typography.semibold}"
    rounded: "{rounded.sm}"
    height: 24px
    padding: 6px
  list-item-selected:
    backgroundColor: "{colors.primary}"
    textColor: "{colors.surface}"
  popup:
    backgroundColor: "{colors.secondary}"
    textColor: "{colors.on-surface}"
    rounded: "{rounded.sm}"
    padding: 4px
elevation:
  raised:
    offset: 2px
    blur: 0.6
    color: "#000000B3"
---

<!---
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Zrythm UI Design Specification

Design specification for the Zrythm QML user interface. This document is the
single source of truth for the design system. The design is dark-first; the
light theme follows the same rules with mirrored token values. The YAML front
matter carries the machine-readable dark-mode tokens; the markdown body is
the human-readable specification.

## Overview

1. **Dark-first** — near-black surfaces (`#000000`–`#161616`) for extended studio
   sessions; a full light theme is derived from the same tokens.
2. **Single accent** — one brand accent (Zrythm orange `#FFAE00` in dark mode,
   celestial blue `#009DFF` in light mode) drives selection, focus, toggled
   states, links and progress.
3. **High-contrast accents** — the accent stands out clearly against dark
   surfaces for selection and playhead indicators.
4. **Compact density** — 24 px control height, 8–14 px type scale, 4 px padding:
   professional audio-tool conventions.
5. **Semantic color roles** — solo (green), record/delete (danger pink), etc.
   map directly to DAW interaction patterns.
6. **Derived states, not new colors** — hover/press/focus colors are computed
   from base tokens by fixed rules (see [State derivation rules](#state-derivation-rules)), so
   every control shifts consistently.
7. **One typeface, many roles** — a single variable font family keeps the UI
   unified while weights and sizes create hierarchy.

## Colors

### Core tokens

Colors are defined as named tokens. Hex values below are exact; modes that
share a value show it once.

| Token | Dark | Light | Role |
|---|---|---|---|
| `primaryColor` | `#FFAE00` | `#009DFF` | Accent: selection, focus, checked/toggled fills, links |
| `zrythmColor` | `#FFAE00` | — | Default dark accent (Zrythm orange) |
| `celestialBlueColor` | `#009DFF` | — | Default light accent fallback |
| `dangerColor` | `#D90368` | — | Destructive actions (record, delete) |
| `soloGreenColor` | `#009B86` | — | Solo state |
| `pageColor` | `#161616` | `#E3E3E3` | Window/panel background |
| `backgroundColor` | `#000000` | `#FFFFFF` | Deepest background layer |
| `alternateBackgroundColor` | `#0F0F0F` | `#D9D9D9` | Alternating row backgrounds (palette `alternateBase`) — the page color recessed by the same perceptual step (≈ 3.7 L*) in both modes |
| `buttonBackgroundColor` | `#323232` | `#CDCDCD` | Control surfaces (buttons, combos, popups) |
| `textColor` | `#E3E3E3` | `#161616` | Primary text; also the *fill* of emphasized buttons (via the `dark` palette role) |
| `placeholderTextColor` | white @ 50 % | black @ 50 % | Text field placeholder |
| `shadowColor` | black @ 70 % | — | Drop shadows |
| `backgroundAppendColor` | white @ 25 % | black @ 25 % | Overlay to make things stand out (menu-bar hover, popup borders) |
| `buttonHoverBackgroundAppendColor` | white @ 10 % | black @ 10 % | Flat (toolbar) button hover overlay |

When switching modes, an accent that only works on one mode falls back
automatically: dark-only accents (`zrythmColor`, `jonquilYellowColor`,
`springGreen`, `munsellRed`) become `celestialBlueColor` in light mode, and the
light-only `gunmetalColor` becomes `zrythmColor` in dark mode.

### Secondary accent colors

| Token | Hex | Availability |
|---|---|---|
| `superorangeColor` | `#FF5500` | both |
| `springGreen` | `#40FFA0` | dark only |
| `jonquilYellowColor` | `#FFD100` | dark only |
| `munsellRed` | `#FF0040` | dark only |
| `electricPurple` | `#A654F7` | both |
| `gunmetalColor` | `#2E3138` | light only |

`superorangeColor` is a high-intensity accent for alerts and strongly active
states; the rest are alternate accent choices.

### State derivation rules

State colors are derived programmatically from the base fill, never
hand-picked:

| State | Fill rule | Border | Content |
|---|---|---|---|
| Default | base token | none (see per-component) | `textColor` |
| Hovered / visual focus | blend toward contrast × 1.3 — dark colors get **lighter**, light colors get **darker** | none | unchanged |
| Pressed (down) | stronger × 1.3 — dark colors get **darker**, light colors get **lighter** | 2 px `highlight` (accent) where the component draws a focus border | text also strengthened |
| Toggled / checked | `accent` fill | none | `brightText` (page color) |
| Disabled | unchanged | unchanged | 70 % opacity |
| Window inactive | unchanged | unchanged | 85 % opacity (60 % if also disabled) |

Example (dark mode button `#323232`): hover ≈ `#414141`, pressed ≈ `#262626`.

Keyboard focus is additionally signalled by a **2 px accent border** on
buttons, combos, tabs, checkboxes, text fields and toolbuttons; at rest,
bordered controls (text fields, checkboxes) use a 1 px `mid` border.

All color and border-width changes animate over **200 ms** with
`Easing.OutExpo` easing.

### Qt palette roles

The tokens map onto the Qt Quick Controls palette roles. Dark-mode values
shown; `blend` means "button background blended toward contrast × 1.3"
(≈ `#414141`).

| Role | Value (dark) | Used for |
|---|---|---|
| `window` / `windowText` | `#161616` / `#E3E3E3` | Page background, label text |
| `button` / `buttonText` | `#323232` / `#E3E3E3` | Control surfaces and their text |
| `base` | `#323232` | Text-editor controls and item views |
| `alternateBase` | `#0F0F0F` | Alternating row backgrounds |
| `light`, `mid`, `midlight` | blend (≈ `#414141`) | Combo hover (`light`), unfocused 1 px borders (`mid`) |
| `dark` | `#E3E3E3` | Emphasized ("Important") button fill |
| `brightText` | `#161616` | Text on accent/highlight fills |
| `highlight` / `highlightedText` | `#FFAE00` / `#161616` | Selection fills and their text |
| `link` / `linkVisited` | `#FFAE00` / `#C48300` | Hyperlinks |
| `placeholderText` | white @ 50 % | Placeholder text |
| `shadow` | black @ 70 % | Shadows |
| `toolTipBase` / `toolTipText` | `#323232` / `#E3E3E3` | Tooltips |

## Typography

* **Family:** Noto Sans (variable font), bundled with the app and chosen for
  its broad glyph coverage across languages. Arabic, Hebrew, Korean,
  Simplified/Traditional Chinese, Thai, and Symbols fallbacks are loaded
  automatically.
* **Base size:** 10 pt (`fontPointSize`); text roles are defined in pixels.
* **Line height:** automatic (font default); text roles do not override it.

| Token | Size | Weight | Used for |
|---|---|---|---|
| `buttonTextFont` | 12 px | Bold | Buttons, combos, toolbuttons |
| `semiBoldTextFont` | 12 px | Medium (500) | Tabs, menu-bar labels, menu items, checkboxes, list rows |
| `normalTextFont` | 12 px | Normal (400) | Labels, text fields, tooltips |
| `trackNameTextFont` | 12 px | Medium (500) | Track headers in the arrangement view |
| `fadedTextFont` | 11 px | Normal | Secondary/disabled text |
| `arrangerObjectTextFont` | 11 px | Medium | Text inside arranger objects |
| `arrangerObjectBoldTextFont` | 11 px | Bold | Emphasized arranger object text |
| `smallTextFont` | 10 px | Normal | Auxiliary info, timestamps |
| `xSmallTextFont` | 9 px | Normal | Fine print, note labels |
| `xxSmallTextFont` | 8 px | Normal | Dense editor annotations |

## Layout & Spacing

Spacing uses a **4 px base unit**: paddings, gaps, and offsets are multiples
of it (4, 8, 12, 16, 24), and the standard control height is 24 px (6 units).

| Token | Value | Purpose |
|---|---|---|
| `buttonHeight` | 24 px | Standard control height (buttons, combos, text fields, menu/list rows) |
| `buttonPadding` | 4 px | Default control padding and spacing |
| `animationDuration` / easing | 200 ms, `OutExpo` | Color/border transitions, popup enter |
| `toolTipDelay` | 700 ms | Hover dwell before tooltip shows |
| `disabledOpacityFactor` / `inactiveOpacityFactor` | 0.7 / 0.85 | See [State derivation rules](#state-derivation-rules) |

Additional fixed metrics: ComboBox implicit width 140; TextField and MenuItem
implicit width 200; progress bar 6 px tall (radii = half the bar height);
check indicator 16 × 16 with 12 px glyph; combo chevron 16 px with 10 px
trailing inset (6 px control padding + 4 px indicator padding);
in-toolbar/menu icons 16 px (24 − 2 × 4); list-row icons 24 px; menu-bar item
padding 12 px left / 16 px right; popup items (menus, combo popups, lists)
are radius 4 with a 6 px text pad (menus 8 px) on 4 px popup content
padding; menu items inset 1 px inside their popup; combo popup keeps 4 px
margin from window edges and 4 px header/footer padding; tooltip gap 3 px.

## Elevation

Depth comes from a single shadow level plus surface/border contrast, not from
a scale of elevations:

| Level | Shadow | Applied to |
|---|---|---|
| Flat | none | Panels, lists, fields, tabs, toolbar buttons at rest on the page |
| Raised | 2 px offset (both axes), blur 0.6, black @ 70 % (`shadowColor`) | Buttons and popup surfaces (tooltips, menus, combo popups) |

## Shapes

The shape language is soft-rounded, never sharp. Corner radii come from a
three-step scale, with fully-round pill ends for tab bars:

| Token | Radius | Used for |
|---|---|---|
| `rounded.sm` (`textFieldRadius`) | 4 px | Text fields, checkboxes, popup surfaces |
| `rounded.md` (`toolButtonRadius`) | 6 px | Toolbar buttons |
| `rounded.lg` (`buttonRadius`) | 9 px | Buttons, combos |
| `rounded.full` | 12 px (half the 24 px control height) | Pill ends: first/last tab outer corners |

Tab bars read as a single pill: only the outer corners of the first and last
tabs are rounded. Progress bar radii are half the bar height (3 px today).

## Components

State tables use the derivation rules from [State derivation rules](#state-derivation-rules);
only deviations are spelled out. Dark-mode values shown.

### Buttons

Rounded (radius 9), 24 px tall, bold 12 px label with a 6 px horizontal
text inset, drop shadow.

| Variant / state | Fill | Text |
|---|---|---|
| Default | `#323232` | `#E3E3E3` |
| Hovered | ≈ `#414141` | unchanged |
| Pressed | ≈ `#262626` + 2 px accent border | strengthened |
| Keyboard focus | ≈ `#414141` + 2 px accent border | unchanged |
| Toggled (checked) | accent `#FFAE00` | dark (`brightText` `#161616`) |
| **Emphasized** (`highlighted`) | near-white `#E3E3E3` (`dark` role) | dark `#161616` |
| **Destructive** | checkable button with `palette.accent`/`palette.buttonText` overridden to `dangerColor` `#D90368`; toggled = danger fill + white text | danger-tinted at rest |

White text is reserved for danger fills.

### Drop Down (ComboBox)

Button-styled closure (radius 9, 140 × 24, bold 12 px) with a 16 px
chevrons-up-down glyph in `textColor`, inset 10 px from the trailing edge
(6 px control padding + 4 px indicator padding).

| State | Appearance |
|---|---|
| Default | button fill `#323232` |
| Hovered | lightened fill |
| Pressed | strengthened fill + 2 px accent border |
| Keyboard focus | 2 px accent border |
| Editable | inset text field matching [Text fields](#text-fields): `base` fill, radius 4, 1 px `mid` border → 2 px accent when focused |

The popup is a `base`-style surface (see [Tooltips and popups](#tooltips-and-popups)) with
4 px content padding that slides in from half its height while fading in
(200 ms, `OutExpo`) and fades out. Items follow the shared item spec
(radius 4, 6 px text pad — see [Lists](#lists)); the **current item is marked
with a check glyph at its trailing edge** in `textColor`, while hover and
keyboard highlight use the accent fill.

### Hyperlinks

Hyperlinks use the palette `link` color. Normal links use the
accent `#FFAE00`; visited links use the darker accent `#C48300`.

### Tabs

The bar reads as one pill: the
first and last tabs round their outer corners at half the height. Labels use
`semiBoldTextFont`.

| State | Fill | Text |
|---|---|---|
| Default | `#323232` | `#E3E3E3` |
| Hovered (unselected) | ≈ `#414141` | unchanged |
| Pressed | strengthened | unchanged |
| Keyboard focus | base + 2 px accent border | unchanged |
| Selected (checked) | accent `#FFAE00` | dark `#161616` |

Tabs activate on drag-hover after a short dwell (drag-to-switch-tab).

### Tooltips and popups

The shared popup surface: `button` fill `#323232`,
radius 4, 1 px `backgroundAppendColor` (white @ 25 %) border, drop shadow.
Used by tooltips and combo/menu popups. Content (menu and combo items) sits
on 4 px padding inside the surface. Dialogs open as native windows: the OS
provides the title bar, frame and shadow, and the dialog paints a flat
window-colored client area inside that frame.

Tooltips: 12 px text in `toolTipText`, 4 px padding, 700 ms
delay, placed 3 px above the control (below if it does not fit), closing on
Escape or click outside.

### Progress bar

6 px tall track in `textColor` and an accent fill, both with radius set to
half the bar height (3 px today). The indeterminate state scrolls five accent
blocks of width/3 in a 1500 ms `InOutQuad` loop.

### Selectable text

Headings and labels use `normalTextFont` in `windowText`. Text selection uses
`selectionColor` = accent and `selectedTextColor` = page color (orange
highlight, dark selected text).

### Checkbox

16 × 16 rounded-square
(radius 4) indicator, `semiBoldTextFont` label, 6 px spacing.

| State | Indicator |
|---|---|
| Unchecked | `base` fill + 1 px `mid` border |
| Hovered | fill blended toward contrast |
| Pressed | strengthened fill |
| Keyboard focus | 2 px accent border |
| Checked | accent `#FFAE00` fill + 12 px dark check glyph |

### Text fields

`base` fill, radius 4, 24 px tall, 200 px
implicit width, `normalTextFont`.

| State | Border | Text |
|---|---|---|
| Default | 1 px `mid` | `textColor` |
| Focused | 2 px accent | `textColor`, selection = accent / page color |
| Placeholder | 1 px `mid` | 50 % white |

### Menu bar

Transparent items, 12 px Medium (`semiBoldTextFont`) label in the
`buttonText` color role, 12 px left / 16 px right padding, 24 px tall.

| State | Fill |
|---|---|
| Default | transparent |
| Hovered | `backgroundAppendColor` overlay (white @ 25 %) |
| Open (down) | accent `#FFAE00` |

### Toolbar buttons

Flat 24 × 24 (radius 6) icon buttons with
16 px icons, bold 12 px labels.

| State | Appearance |
|---|---|
| Default | transparent |
| Hovered / pressed | `buttonHoverBackgroundAppendColor` overlay (white @ 10 %) |
| Keyboard focus | 2 px accent border |
| Toggled (checked) | accent-colored icon and text (no fill) |

### Menus and menu items

Menus and menu items sit on the shared popup surface. Items
are 24 px tall, radius 4, inset 1 px inside the popup with an 8 px text pad,
`semiBoldTextFont`; checkable items show a check glyph, submenus an arrow.
Items with an associated shortcut show it right-aligned in the trailing
padding in `fadedTextFont` (11 px Normal) at 62 % text opacity; submenu
rows and rows without a shortcut reserve nothing.

| State | Fill | Text |
|---|---|---|
| Default | transparent | `windowText` |
| Highlighted (hover/selection) | accent `#FFAE00` | dark `#161616` |
| Pressed | strengthened accent | dark |

### Lists

24 px rows
with radius 4 and a 6 px text pad, `semiBoldTextFont`, 24 px icons; section
headers use bold labels (list heading). This is the shared item spec also
used by combo popup and menu items. Alternating row backgrounds use
`alternateBackgroundColor` (see [Core tokens](#core-tokens)).

| State | Fill | Text |
|---|---|---|
| Default | transparent | `textColor` |
| Hovered | contrast-blended button ≈ `#414141` | unchanged |
| Selected (highlighted) | accent `#FFAE00` | dark `#161616` |
| Pressed | strengthened | unchanged |

### Selection

Selected arranger objects (clips, chords, MIDI notes, automation points,
markers, and tempo-map badges) draw a 1 px `textColor` outline around
their bounds, following each object's corner radius (fully round for
automation points). On top of the outline, the fill brightens slightly
toward contrast and the object name is rendered bold.

## Do's and Don'ts

- Do use the accent for selection, keyboard focus, toggled states, links, and
  at most one primary action per view.
- Don't introduce one-off colors; derive hover/press states with the rules in
  [State derivation rules](#state-derivation-rules) or add a named token.
- Do keep controls on the 4 px spacing grid and the 24 px control height.
- Don't mix corner radii: 9 px buttons/combos/tabs, 6 px toolbuttons, 4 px
  text fields, checkboxes, and popups.
- Do use dark text (`brightText` / page color) on accent fills.
- Don't use `dangerColor` outside destructive contexts (record, delete).
- Do animate color and border-width changes at 200 ms `OutExpo`.
- Don't hardcode color values in components; consume theme tokens or
  palette roles.

## Theme Switching

The theme can be flipped between dark and light at runtime; every token,
palette role, and derived state follows automatically. Accent fallbacks
between modes are described in [Core tokens](#core-tokens).
