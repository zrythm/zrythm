// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// zrythm color variant (accent | accent lighter | accent darker):
// F79616 | FFA533 | D68A0C
// more prominent: FF9F00 | FFB300 | F79616

pragma Singleton

import QtQuick
import QtQuick.Templates as T
import ZrythmUtils

QtObject {
  id: root

  readonly property real animationDuration: 200
  readonly property int animationEasingType: Easing.OutExpo
  property color alternateBackgroundColor: darkMode ? "#0F0F0F" : "#D9D9D9"
  readonly property font arrangerObjectBoldTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 11,
      "weight": Font.Bold
    })
  readonly property font arrangerObjectTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 11,
      "weight": Font.Medium
    })
  property color backgroundAppendColor: darkMode ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(0, 0, 0, 0.25) // color to be appended to things that stand out
  property color backgroundColor: darkMode ? "#000000" : "#FFFFFF"
  property color buttonBackgroundColor: darkMode ? "#323232" : "#CDCDCD"
  readonly property real buttonHeight: 24
  property color buttonHoverBackgroundAppendColor: darkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.1)
  readonly property real buttonPadding: 4
  readonly property real buttonRadius: 9
  readonly property font buttonTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 12,
      "bold": true
    })
  readonly property color celestialBlueColor: "#009DFF"
  // The two polarity inks a fill's text/glyph may take; the one passing
  // contrast against the fill is chosen by polarityTextColor()
  // (DESIGN.md "Buttons")
  readonly property color lightInkColor: "#FFFFFF"
  readonly property color darkInkColor: "#161616"
  // 9 px bold letter glyphs for channel state buttons (DESIGN.md
  // "Channel state buttons")
  readonly property font channelStateTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 9,
      "bold": true
    })
  // Body fill opacity of the arranger color recipe, shared by clip
  // bodies and label-object surfaces over the lane grid; a
  // transparent body washes out its chroma too, so this stays well
  // above half (DESIGN.md "Clips")
  readonly property real bodyAlpha: 0.75
  // Whole-clip opacity of clips on muted tracks (DESIGN.md "Clips")
  readonly property real mutedClipOpacity: 0.5
  // Height of dense inline controls (channel state chips and compact
  // word-mode selectors); DESIGN.md "Channel state buttons"
  readonly property real compactControlHeight: 18
  readonly property Palette colorPalette: Palette {
    accent: root.primaryColor
    alternateBase: root.alternateBackgroundColor
    base: root.buttonBackgroundColor // background color for text editor controls and item views
    brightText: root.pageColor
    button: root.buttonBackgroundColor
    buttonText: root.textColor
    dark: root.textColor // used by paginator, docs mention a "foreground" color
    highlight: root.primaryColor
    highlightedText: root.pageColor
    light: root.getColorBlendedTowardsContrast(root.buttonBackgroundColor) // "lighter than button background" / combo box hover background
    link: root.primaryColor
    linkVisited: root.getStrongerColor(root.primaryColor)
    mid: root.getColorBlendedTowardsContrast(root.buttonBackgroundColor) // docs say "between button background color and text color", no idea where it's used
    midlight: root.getColorBlendedTowardsContrast(root.buttonBackgroundColor)
    placeholderText: root.placeholderTextColor
    shadow: root.shadowColor
    text: root.textColor
    toolTipBase: root.buttonBackgroundColor
    toolTipText: root.textColor
    window: root.pageColor
    windowText: root.textColor
  }
  property bool darkMode: true
  readonly property var darkOnlyThemeColors: [zrythmColor, jonquilYellowColor, springGreen, munsellRed]
  readonly property real disabledOpacityFactor: 0.7
  readonly property real downEnhancementFactor: lightenFactor // enhance things pressed down by 30%
  readonly   property color electricPurple: "#A654F7"
  property color errorColor: darkMode ? "#FF4747" : "#A30015"

  // the error hue at 4.5:1 as resting text
  readonly property color errorTextColor: darkMode ? "#FF6862" : errorColor
  readonly property font fadedTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 11,
      "weight": Font.Normal
    })
  property string fontFamily: notoSansFont.name
  readonly property real fontPointSize: 10
  readonly property color gunmetalColor: "#2E3138"
  readonly property real inactiveOpacityFactor: 0.85
  readonly property color jonquilYellowColor: "#FFD100"
  readonly property var lightOnlyThemeColors: [gunmetalColor]
  readonly property real lightenFactor: 1.3 // lighten things up 10%, mainly used for hovering but can be used for other things like making parts of the UI stand out from the background
  readonly   property color munsellRed: "#FF0040"
  // Notification surfaces: the severity hue at a fixed lightness per mode
  // (see DESIGN.md, Notifications); info uses a neutral surface
  property color notificationSurfaceErrorColor: darkMode ? "#421C19" : "#FFCFCA"
  property color notificationSurfaceInfoColor: darkMode ? "#292929" : "#DEDEDE"
  property color notificationSurfaceSuccessColor: darkMode ? "#003229" : "#B4ECDE"
  property color notificationSurfaceWarningColor: darkMode ? "#332800" : "#ECDEB1"
  readonly property font normalTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 12,
      "weight": Font.Normal
    })
  readonly property FontLoader notoSansArabicFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSansArabic-VariableFont_wdth_wght.ttf")
  }

  // These are used as fallbacks automatically just by declaring them
  // even if they are not referenced anywhere explicitly
  readonly property FontLoader notoSansFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSans-VariableFont_wdth_wght.ttf")
  }
  readonly property FontLoader notoSansHebrewFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSansHebrew-VariableFont_wdth_wght.ttf")
  }
  readonly property FontLoader notoSansItalicFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSans-Italic-VariableFont_wdth_wght.ttf")
  }
  readonly property FontLoader notoSansKrFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSansKR-VariableFont_wght.ttf")
  }
  readonly property FontLoader notoSansScFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSansSC-VariableFont_wght.ttf")
  }
  readonly property FontLoader notoSansSymbols2Font: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSansSymbols2-Regular.ttf")
  }
  readonly property FontLoader notoSansSymbolsFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSansSymbols-VariableFont_wght.ttf")
  }
  readonly property FontLoader notoSansTcFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSansTC-VariableFont_wght.ttf")
  }
  readonly property FontLoader notoSansThaiFont: FontLoader {
    source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/fonts/NotoSansThai-VariableFont_wdth_wght.ttf")
  }
  // used in contrast with textColor

  // The light-mode page color; the reference the light-mode color
  // derivations measure their contrast against
  readonly property color lightPageColor: "#E3E3E3"
  property color pageColor: darkMode ? "#161616" : lightPageColor
  property color placeholderTextColor: darkMode ? Qt.rgba(1, 1, 1, 0.5) : Qt.rgba(0, 0, 0, 0.5)
  property color primaryColor: zrythmColor

  property color recordColor: darkMode ? "#D90368" : "#9E004A"

  // the record hue at 4.5:1 as resting text
  readonly property color recordTextColor: darkMode ? "#FF5F9E" : recordColor

  readonly property PropertyAnimation propertyAnimation: PropertyAnimation {
    duration: root.animationDuration
    easing.type: root.animationEasingType
  }
  readonly property real scrollLoaderBufferPx: 64
  readonly property font semiBoldTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 12,
      "weight": Font.Medium
    })
  readonly property color shadowColor: Qt.rgba(0, 0, 0, 0.7) // Qt.alpha(backgroundColor, 0.17)
  readonly property font smallTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 10,
      "weight": Font.Normal
    })
  property color soloGreenColor: darkMode ? "#009B86" : "#006456"
  readonly property color springGreen: "#40FFA0"
  readonly property color successColor: darkMode ? "#009B86" : "#006456"
  readonly property color superorangeColor: "#FF5500"
  property color textColor: darkMode ? "#E3E3E3" : "#161616" // used in contrast with pageColor
  readonly property font trackNameTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 12,
      "weight": Font.Medium
    })
  readonly property real textFieldRadius: 4
  readonly property real toolButtonRadius: 6
  readonly property int toolTipDelay: 700
  property color warningColor: darkMode ? "#FFD100" : "#675300"
  readonly property font xSmallTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 9,
      "weight": Font.Normal
    })
  readonly property font xxSmallTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 8,
      "weight": Font.Normal
    })
  readonly property color zrythmColor: "#FFAE00"

  function adjustColorForHoverOrVisualFocusOrDown(arg: color, hovered: bool, focused: bool, down: bool): color {
    if (!hovered && !focused && !down)
      return arg;
    else if (down)
      return root.getStrongerColor(arg);
    else if (hovered || focused)
      return root.getColorBlendedTowardsContrast(arg);
  }

  // Text and icon ink for buttons: hovered and keyboard-focused text
  // stays unchanged, pressed text strengthens (DESIGN.md "State
  // derivation rules"); checked and highlighted take their fill's
  // polarity.
  function buttonContentColor(control: T.Button): color {
    const baseColor = control.checked
      ? root.polarityTextColor(control.palette.accent)
      : control.highlighted ? control.palette.brightText
      : (control.flat && !control.down)
        ? (control.visualFocus ? control.palette.highlight : control.palette.windowText)
        : control.palette.buttonText;
    return root.adjustColorForHoverOrVisualFocusOrDown(baseColor, false, false, control.down);
  }

  // Makes dark colors lighter and light colors darker by the lighten factor.
  function getColorBlendedTowardsContrast(arg: color): color {
    return getColorBlendedTowardsContrastByFactor(arg, root.lightenFactor);
  }

  function getColorBlendedTowardsContrastByFactor(arg: color, factor: real): color {
    return root.isColorDark(arg) ? arg.lighter(factor) : arg.darker(factor);
  }

  function getOpacity(enabled: bool, windowActive: bool): real {
    if (enabled && windowActive)
      return 1;
    else if (enabled && !windowActive)
      return root.inactiveOpacityFactor;
    else if (!enabled && windowActive)
      return root.disabledOpacityFactor;
    else if (!enabled && !windowActive)
      return root.disabledOpacityFactor - 0.1;
  }

  // Makes dark colors darker and light colors lighter by the down enhancement factor.
  function getStrongerColor(arg: color): color {
    if (root.isColorDark(arg))
      return arg.darker(root.downEnhancementFactor);
    const lightened = arg.lighter(root.downEnhancementFactor);
    // lighter() clamps at maximum brightness, in which case darkening is
    // used to strengthen the color
    return lightened == arg ? arg.darker(root.downEnhancementFactor) : lightened;
  }

  function isColorDark(arg: color): bool {
    return arg.hslLightness < 0.5;
  }

  // WCAG 2.x contrast ratio between two colors (1 to 21)
  function contrastRatio(a: color, b: color): real {
    return ColorMath.contrastRatio(a, b);
  }

  // sRGB color converted to OKLCH as [L, chroma, hue in degrees]
  function rgbToOklch(c: color): var {
    return ColorMath.rgbToOklch(c);
  }

  // OKLCH converted back to sRGB, with the chroma reduced until the
  // color fits the sRGB gamut
  function oklchColor(l: real, c: real, h: real): color {
    return ColorMath.oklchColor(l, c, h);
  }

  // A palette of @p total colors with hues evenly spread from the
  // accent hue: slot 0 shares the accent's hue, and slot i steps by
  // 360/@p total degrees. Fills keep the vivid base lightness in both
  // modes; in light mode the lightness shifts darker away from the
  // light surface only until the fill passes 3:1 against it
  // (DESIGN.md "Colors", non-text elements / mode-exempt hues).
  function distinctColor(index: int, total: int): color {
    if (total <= 0)
      return primaryColor;
    const baseHue = rgbToOklch(primaryColor)[2];
    const hue = (baseHue + index * 360 / total) % 360;
    return ColorMath.oklchColorWithContrast(
      hue, 0.15, 0.72, darkMode, lightPageColor, 3.0, 0.45);
  }

  // Base surface color with selection and hover overlays blended in:
  // 20% of the ink for selection, 5% for hover. The result is opaque.
  function surfaceColor (
    baseColor: color, inkColor: color, isSelected: bool,
    isHovered: bool): color {
    let c = baseColor;
    if (isSelected) {
      c = _blend(c, inkColor, 0.20);
    }
    if (isHovered) {
      c = _blend(c, inkColor, 0.05);
    }
    return c;
  }

  function _blend(base: color, overlay: color, amount: real): color {
    const inv = 1.0 - amount;
    return Qt.rgba(
      base.r * inv + overlay.r * amount,
      base.g * inv + overlay.g * amount,
      base.b * inv + overlay.b * amount, 1);
  }

  // The arranger color recipe's OKLCH anchors (DESIGN.md "Clips").
  // Every surface keeps the source hue and pins it to a fixed
  // lightness/chroma pair per theme, given as [dark, light]; the
  // border and the name scrim shift the body lightness by a delta.
  readonly property var _recipe: {
    // Content ink: the deepest chroma holding >= 3:1 against every
    // body the recipe produces
    "inkL": [0.60, 0.44],
    "inkC": [0.21, 0.18],
    // Quiet surface shared by clip bodies and label objects
    "bodyL": [0.30, 0.89],
    "bodyC": [0.065, 0.06],
    "borderDeltaL": [0.08, -0.05],
    "scrimDeltaL": [-0.08, -0.08],
    // HSL saturation below which a source renders gray
    "achromaticSaturation": 0.05,
  }

  // All recipe surfaces for a source color in one OKLCH pass
  function _recipeColors (source: color, forceAchromatic: bool): var {
    const [, , hue] = rgbToOklch (source);
    const achromatic = forceAchromatic
      || source.hslSaturation < _recipe.achromaticSaturation;
    const mode = darkMode ? 0 : 1;
    const bodyL = _recipe.bodyL[mode];
    const bodyC = achromatic ? 0 : _recipe.bodyC[mode];
    const inkC = achromatic ? 0 : _recipe.inkC[mode];
    return {
      body: oklchColor (bodyL, bodyC, hue),
      border: oklchColor (bodyL + _recipe.borderDeltaL[mode], bodyC, hue),
      scrim: oklchColor (bodyL + _recipe.scrimDeltaL[mode], bodyC, hue),
      ink: oklchColor (_recipe.inkL[mode], inkC, hue),
    };
  }

  // Musical-content color for arranger objects outside clips (notes,
  // chords, automation curves, editor waveforms): the source's hue at
  // the clip content ink's lightness and chroma, so in-clip and
  // out-of-clip content render identically (DESIGN.md "Clips").
  // Achromatic sources yield gray.
  function contentColor (source: color, forceAchromatic: bool): color {
    return _recipeColors (source, forceAchromatic).ink;
  }

  // Label-object surface colors (markers, tempo/time-signature
  // objects): the clip body treatment — the source's hue at the clip
  // body's lightness and chroma. Achromatic sources yield gray.
  // Returns { body, border }.
  function labelColors (source: color, forceAchromatic: bool): var {
    const colors = _recipeColors (source, forceAchromatic);
    return { body: colors.body, border: colors.border };
  }

  // Clip surface colors derived from the clip/track color in a single
  // pass (DESIGN.md "Clips"): the source's hue at the recipe lightness
  // and chroma, for the body, border, name scrim and content ink.
  // Achromatic sources yield gray. Ink sits at a fixed lightness
  // holding ≥ 3:1 (WCAG graphics contrast) against every body the
  // recipe can produce; text on the body takes the polarity rule
  // instead (polarityTextColor). Muted clips come out fully
  // achromatic — every part at zero chroma — so the loss of color
  // itself signals that the clip is not playing. Returns
  // { body, border, scrim, ink }.
  function clipColors (source: color, muted: bool): var {
    return _recipeColors (source, muted);
  }

  // The ink polarity that passes contrast against the given fill: white
  // or #161616 — whichever reaches 4.5:1 against it, the higher ratio
  // winning if neither does (DESIGN.md "Buttons")
  function polarityTextColor(fill: color): color {
    const whiteRatio = contrastRatio(root.lightInkColor, fill);
    const blackRatio = contrastRatio(root.darkInkColor, fill);
    if (whiteRatio >= 4.5 && whiteRatio >= blackRatio)
      return root.lightInkColor;
    if (blackRatio >= 4.5)
      return root.darkInkColor;
    return whiteRatio >= blackRatio ? root.lightInkColor : root.darkInkColor;
  }

  function toggleDarkMode(): void {
    darkMode = !darkMode;
    if (darkMode && lightOnlyThemeColors.includes(primaryColor)) {
      primaryColor = zrythmColor;
    } else if (
    // fall back to a light-mode-compatible color when switching to light
    !darkMode && darkOnlyThemeColors.includes(primaryColor)) {
      primaryColor = celestialBlueColor;
    }
  }

  Component.onCompleted: {}
}
