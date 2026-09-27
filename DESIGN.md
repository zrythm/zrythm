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
  record: "#D90368"
  solo-green: "#009B86"
  success: "#009B86"
  warning: "#FFD100"
  error: "#FF4747"
  notification-surface-info: "#292929"
  notification-surface-success: "#003229"
  notification-surface-warning: "#332800"
  notification-surface-error: "#421C19"
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
  floating-surface-width: 360px
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
  button-destructive-toggled:
    backgroundColor: "{colors.error}"
    textColor: "{colors.surface}"
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
  dialog:
    backgroundColor: "{colors.surface}"
    padding: 12px
  alert-dialog:
    width: "{spacing.floating-surface-width}"
  toast:
    width: "{spacing.floating-surface-width}"
    padding: 12px
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

Scope note: this document records tokens, reusable conventions, and
accessibility contracts. Per-component realization arithmetic — padding
sums, inset offsets, one-off geometry no other surface reuses — lives in
the components' code; canonical proportions more than one surface shares
(control heights, floating-surface widths) are tokens and stay here.

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
5. **Semantic color roles** — solo (green), record (pink), failure and
   destructive actions (red) — map directly to DAW interaction patterns.
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
| `primaryColor` | `#FFAE00` | `#009DFF` | Accent: selection, focus, checked/toggled fills |
| `zrythmColor` | `#FFAE00` | — | Default dark accent (Zrythm orange) |
| `celestialBlueColor` | `#009DFF` | — | Default light accent fallback |
| `recordColor` | `#D90368` | `#9E004A` | Recording workflow (record buttons, armed states, playhead while recording) |
| `soloGreenColor` | `#009B86` | `#006456` | Solo state |
| `successColor` | `#009B86` | `#006456` | Positive confirmations (export finished, project saved) |
| `warningColor` | `#FFD100` | `#675300` | Recoverable problems (device fallback, refused operation) |
| `errorColor` | `#FF4747` | `#A30015` | Failures (plugin crashed, export failed), destructive actions (delete, Don't Save), and over-level metering |
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

`recordColor`, `soloGreenColor`, and the severity colors (`successColor`,
`warningColor`, `errorColor`) are exempt from this fallback: their hues —
and therefore their meaning — are fixed across modes. Their light values
derive mechanically from the dark ones — same OKLCH hue, maximum in-gamut
chroma at a fixed OKLCH lightness of 0.45.
Value collisions with other tokens (`successColor` = `soloGreenColor` in
both modes, `warningColor` = `jonquilYellowColor` in dark mode) are
deliberate; the tokens are semantically independent and may diverge.

Text and meaningful glyphs reach ≥4.5:1 against the surface beneath;
indicator fills, bars, and lines — non-text elements — reach 3:1.
When a mode-exempt hue (`recordColor`, `soloGreenColor`, the severity
colors) misses that as resting text or glyphs, keep its OKLCH hue and
chroma (within gamut) and shift the lightness away from the surface
until it passes; fills keep the base value and take polarity text
([Buttons](#buttons)).

### Secondary accent colors

| Token | Hex | Availability |
|---|---|---|
| `superorangeColor` | `#FF5500` | both |
| `springGreen` | `#40FFA0` | dark only |
| `jonquilYellowColor` | `#FFD100` | dark only |
| `munsellRed` | `#FF0040` | dark only |
| `electricPurple` | `#A654F7` | both |
| `gunmetalColor` | `#2E3138` | light only |

`superorangeColor` is a high-intensity accent for strongly active
states; the rest are alternate accent choices.

### State derivation rules

State colors derive programmatically from the base fill:

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
| `link` / `linkVisited` | `#FFAE00` / `#DBBB8F` | Hyperlinks (derived — see [Hyperlinks](#hyperlinks)) |
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

When the platform requests reduced motion, slide and move animations are
skipped in favor of instant changes or simple fades; continuous
functional animations, such as an indeterminate progress bar, keep
running.

## Elevation

Depth comes from a single shadow level plus surface/border contrast:

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
| **Destructive** | base button fill at rest, error fill when toggled | adjusted error hue at rest, polarity text when toggled |

Fill text is white or `#161616` — whichever reaches ≥4.5:1 against the
fill.

### Drop Down (ComboBox)

Button-styled closure (radius 9, 140 × 24, bold 12 px) with a 16 px
chevrons-up-down glyph in `textColor`, inset 10 px from the trailing edge.

| State | Appearance |
|---|---|
| Default | button fill `#323232` |
| Hovered | lightened fill |
| Pressed | strengthened fill + 2 px accent border |
| Keyboard focus | 2 px accent border |
| Editable | inset text field matching [Text fields](#text-fields): `base` fill, radius 4, 1 px `mid` border → 2 px accent when focused |

The popup is a `base`-style surface (see
[Tooltips and popups](#tooltips-and-popups)) with 4 px content padding,
kept 4 px from the window edges, that slides in from half its height
while fading in (200 ms, `OutExpo`) and fades out. Items follow the
shared item spec (radius 4, 6 px text pad — see [Lists](#lists)); the
**current item is marked
with a check glyph at its trailing edge** in `textColor`, while hover and
keyboard highlight use the accent fill.

### Hyperlinks

Hyperlinks use the palette `link` color, derived from the accent in
both modes: the accent itself where it passes 4.5:1 as text on the page
(`#FFAE00` in dark mode), otherwise the accent hue at the severity-token
lightness (OKLCH L 0.45, maximum in-gamut chroma — `#005894` in light
mode). Visited links keep the link's hue and lightness and reduce the
chroma to 40 % — a mechanical fade that preserves contrast on every
surface for every accent. Inline links render underlined; button-like
link affordances (the toast action) do not. Selection, focus, and
toggled fills keep the full-strength accent in both modes. Links render
on page surfaces.

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
on 4 px padding inside the surface.

Tooltips: 12 px text in `toolTipText`, 4 px padding, 700 ms
delay, placed 3 px above the control (below if it does not fit).

### Dialogs

Dialogs open as native windows: the OS provides the title bar, frame and
shadow, and the dialog paints a flat surface-colored client area inside
that frame. Alerts fill that client area with the layout below. Content
padding is 12 px on the spacing grid; standard dialogs carry their title
in the OS title bar and the client area does not repeat it — alerts are
the exception, their heading and window title mirror each other.

**Alerts** — confirmations and the notification Interruption tier —
show a 16 px severity glyph in the full-strength severity token at the
leading edge of a `semiBoldTextFont` heading with the message in
`normalTextFont` spanning the full content width beneath the glyph.
Alerts are 360 px wide.

**Buttons** sit in a trailing-aligned row, mirrored in RTL: accept in the
trailing slot, cancel next to it, further actions leading. The accept
button is the dialog's default — Return activates it — and renders in
the Emphasized variant; destructive actions render in the Destructive
variant and are never the default, so when the accept action is
destructive, the safe action takes the default. Escape activates
cancel; a dialog without one dismisses, activating no button. Initial
focus goes to the first text field if present, else the default button;
dialogs take focus on appearance. While a modal dialog is open, the
parent window paints a black @ 35 % scrim that fades with the standard
transition.

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
| Invalid | 2 px `errorColor` | `textColor` |
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

Two-line list rows share one row anatomy: 40 px rows with a medium title
and a small subtitle at 55 % opacity — falling back to a static string
(e.g. a type or format label) when no subtitle exists, never an empty
second line — plus 16 px leading glyphs and alternating
`alternateBackgroundColor` fills. The 40 px height is a deliberate
exception to the 24 px row metric for two-line rows. Suffix action buttons
follow the toolbar-button rules (24 px hit area, 16 px glyph, radius 6,
white @ 10 % hover overlay) and are revealed on row hover or keyboard
focus; a checked action stays visible at rest. Additional actions live
behind a "…" button that opens the shared menu. Selection follows the
table above, with all content — text and glyphs — switching to dark on
the accent fill. Rows also serve as plain setting rows (title + subtitle
+ always-visible suffix controls, no interactions) in preferences-style
forms.

### Notifications

User-facing events surface through a three-tier model. Toasts are the
attention layer, the notification center (bell + history popover) is the
memory layer, and modal dialogs stay reserved for blocking errors:

| Tier | Surface | Gets |
|---|---|---|
| Attention | toast (transient) | info, success, warning, error |
| Memory | bell + history popover | every notification is recorded here, including those surfaced as modals |
| Interruption | modal dialog | critical only |

**Toast anatomy.** A floating popup whose surface is defined directly
in OKLCH: the severity hue at lightness 0.28 in dark mode and 0.90 in
light mode, chroma 0.06 clipped to the sRGB gamut in both modes to keep
perceived saturation consistent across themes; info has no severity hue
and uses a neutral gray at the same lightness. Severity surfaces
separate from the page by hue in addition to the shared popup recipe
(4 px radius, raised shadow) with one deviation: a 1 px `mid` border in
place of the shared popup's `backgroundAppendColor` — a stronger edge
for a surface floating over arbitrary content; the neutral info
surface separates by the border and shadow alone, like every popup
surface. A 16 px severity glyph (aligned with the title line) takes the
severity hue with the lightness adjustment applied where the surface
requires it; the bell badge fill keeps the full-strength token. The
title uses
`semiBoldTextFont` and elides after one line; an optional detail line
uses `fadedTextFont`
(wrapped, elided after ~3 lines to accommodate translation expansion,
spanning the full content width beneath the glyph),
coalesced events show a "×N" chip after the title (`smallTextFont` on a
solid `mid` pill), and a low-emphasis
close button (real and focusable) sits on the trailing edge of the
title line. An
optional single action — a real, focusable text button in the palette
`link` color — may precede the close button; activating it performs the
action and dismisses the toast. Secondary
text — the detail line and the chip — renders `textColor` at 70 %
opacity, which keeps ≥4.5:1 contrast on every toast background. Toasts
are 360 px wide — the shared floating-surface width — with 12 px content
padding; the close button is a 24 × 24 flat target with a 16 px
`cross-small-symbolic` glyph.

**Severity language:**

| Severity | Glyph | Glyph color | Background (dark / light) | Lifetime |
|---|---|---|---|---|
| Info | `info-outline-symbolic` | `textColor` | `#292929` / `#DEDEDE` | 5 s |
| Success | `check-round-outline-symbolic` | `successColor` | `#003229` / `#B4ECDE` | 5 s |
| Warning | `warning-outline-symbolic` | `warningColor` | `#332800` / `#ECDEB1` | 10 s |
| Error | `cross-small-circle-outline-symbolic` | `errorColor` | `#421C19` / `#FFCFCA` | 30 s |
| Critical | `cross-small-circle-outline-symbolic` | `errorColor` | — | modal dialog |

`recordColor` and `errorColor` are distinct: record marks the *capture
intent* of the user — recording is a mode, not a problem — while error
marks a *failure* that has already happened or a destructive action
*about to* happen (delete, Don't Save).
Critical marks failures that block further work or risk data loss
(project failed to load, audio device lost); it interrupts via modal
dialog and never appears as a toast.

**Behavior.** Toasts enter by sliding down 8 px from beneath the
toolbar while fading in (200 ms `OutExpo` — the popup-enter convention)
and exit with a plain fade. Up to 3 are visible at once, newest on top;
further events queue and appear in arrival order as slots free up — no
severity preempts another, and every event keeps its place (an error
holds its slot for its full 30 s lifetime). Two events
coalesce when they share severity, title and context tag: the visible
toast's ×N count is incremented and its dismissal timer restarts.
Queued events coalesce the same way, bumping the queued count. A
context tag identifies the event's source and is assigned by the
producer (e.g. `plugin:<uuid>`, `export`). Hovering the toast — or
focusing its close button — pauses the dismissal timer; clicking the
body or the close button dismisses. Toasts never take focus on
appearance; the close button remains tab-reachable, and toast buttons
follow the main content in the tab order. `Escape` dismisses
the topmost toast when no menu, popup, or dialog is open. Every
notification is recorded in the history (including those surfaced as
modals), so a missed, expired, or queued toast loses no information —
the badge
on the bell keeps unseen warnings and errors visible until the toast is
dismissed or the popover is opened. Toasts sit in the top trailing corner, just below the main
toolbar, aligned 16 px from the trailing window edge (top-right in
left-to-right layouts, mirrored in RTL), and expose their title as the
accessible name and their detail as the accessible description. Toasts
are announced by the platform screen-reader announcement mechanism when
they appear; no focus change is involved.

**Notification center.** A bell (`bell-outline-symbolic`) at the
trailing end of the main toolbar badges the number of unacknowledged
warning/error occurrences — the history keeps one row per occurrence,
so a recurring problem counts once per unseen event — filled with the
color of the highest unacknowledged
severity and capped
at "9+". Critical events never badge the bell — dismissing their modal
acknowledges them; they appear in the history like every other event.
The numeral keeps ≥4.5:1 against its fill in every combination (see
[Buttons](#buttons)). Its popover, at the shared floating-surface width,
lists the retained
history — the most recent 100
events, session-only, not persisted across restarts — on the standard
popup surface: severity glyph, title, elided detail, locale-aware
relative timestamp, and a Clear All action (immediate, not
undoable). Opening it marks everything seen. Explicitly dismissing a
toast — close button, click, Escape, or its action — acknowledges
every occurrence it absorbed; expiry does not acknowledge. Events
arriving while the popover is open join the history immediately and are
marked seen when it closes.

### Selection

Selected arranger objects (clips, chords, MIDI notes, automation points,
markers, and tempo-map badges) draw a 1 px `textColor` outline around
their bounds, following each object's corner radius (fully round for
automation points). On top of the outline, the fill brightens slightly
toward contrast and the object name is rendered bold.

### Playhead

A 2 px full-height accent line in the arranger; it switches to
`recordColor` while the transport records. A same-color glow hugs the
line — accent at rest, `recordColor` while recording — and a small cap
at the ruler marks its
position. While the transport records, the record indicators in the
transport buttons blink: one 1 s ease-in-out cycle between full and
25 % opacity and back, continuing under reduced motion.

## Do's and Don'ts

- Do use the accent for selection, keyboard focus, toggled states, and
  at most one primary action per view.
- Do make the accept button the dialog's only emphasized button;
  destructive actions are never the default.
- Don't introduce one-off colors; derive hover/press states with the rules in
  [State derivation rules](#state-derivation-rules) or add a named token.
- Do keep controls on the 4 px spacing grid and the 24 px control height.
- Don't mix corner radii: 9 px buttons/combos/tabs, 6 px toolbuttons, 4 px
  text fields, checkboxes, and popups.
- Do give accent fills the text polarity that passes AA — page color in
  dark mode, base text color in light mode.
- Don't use `recordColor` outside recording contexts (record buttons, the
  playhead while recording, armed states).
- Severity colors (`successColor`, `warningColor`, `errorColor`) are
  reserved for notification surfaces (toasts, history rows, alert
  dialogs, severity glyphs, bell badge), destructive actions
  (destructive buttons, delete/cut tool feedback), over-level metering,
  and inline form validation.
- Do animate color and border-width changes at 200 ms `OutExpo`.
- Don't hardcode color values in components; consume theme tokens or
  palette roles.

## Theme Switching

The theme can be flipped between dark and light at runtime; every token,
palette role, and derived state follows automatically. Accent fallbacks
between modes are described in [Core tokens](#core-tokens).
