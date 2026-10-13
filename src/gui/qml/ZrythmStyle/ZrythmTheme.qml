// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// zrythm color variant (accent | accent lighter | accent darker):
// F79616 | FFA533 | D68A0C
// more prominent: FF9F00 | FFB300 | F79616

pragma Singleton

import QtQuick
import QtQuick.Templates as T

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

  property color pageColor: darkMode ? "#161616" : "#E3E3E3"
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

  // WCAG 2.x relative luminance of a color
  function luminance(arg: color): real {
    return 0.2126 * linearChannel(arg.r) + 0.7152 * linearChannel(arg.g) + 0.0722 * linearChannel(arg.b);
  }

  // A color channel converted to its WCAG 2.x linear-light value
  function linearChannel(v: real): real {
    return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4);
  }

  // WCAG 2.x contrast ratio between two colors (1 to 21)
  function contrastRatio(a: color, b: color): real {
    const la = luminance(a);
    const lb = luminance(b);
    return (Math.max(la, lb) + 0.05) / (Math.min(la, lb) + 0.05);
  }

  // A linear-light channel converted back to its sRGB value
  function srgbChannel(v: real): real {
    return v <= 0.0031308 ? 12.92 * v : Math.min(1.0, 1.055 * Math.pow(v, 1 / 2.4) - 0.055);
  }

  // sRGB color converted to OKLCH as [L, chroma, hue in degrees].
  // Coefficients from Ottosson's OKLab reference implementation
  // (bottosson.github.io/posts/oklab)
  function rgbToOklch(c: color): var {
    const r = linearChannel(c.r);
    const g = linearChannel(c.g);
    const b = linearChannel(c.b);
    const lms0 = Math.cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    const lms1 = Math.cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    const lms2 = Math.cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
    const lightness = 0.2104542553 * lms0 + 0.7936177850 * lms1 - 0.0040720468 * lms2;
    const a = 1.9779984951 * lms0 - 2.4285922050 * lms1 + 0.4505937099 * lms2;
    const bb = 0.0259040371 * lms0 + 0.7827717662 * lms1 - 0.8086757660 * lms2;
    const chroma = Math.sqrt(a * a + bb * bb);
    let hue = Math.atan2(bb, a) * 180 / Math.PI;
    if (hue < 0)
      hue += 360;
    return [lightness, chroma, hue];
  }

  // OKLCH converted back to sRGB, with the chroma reduced until the
  // color fits the sRGB gamut. Coefficients from Ottosson's OKLab
  // reference implementation (bottosson.github.io/posts/oklab)
  function oklchColor(l: real, c: real, h: real): color {
    const toLinear = (ll, aa, bb) => {
      const lms0 = Math.pow(0.9999999985 * ll + 0.3963377922 * aa + 0.2158037581 * bb, 3);
      const lms1 = Math.pow(1.0000000089 * ll - 0.1055613423 * aa - 0.0638541748 * bb, 3);
      const lms2 = Math.pow(1.0000000547 * ll - 0.0894841821 * aa - 1.2914855379 * bb, 3);
      return [
        4.0767416613 * lms0 - 3.3077115904 * lms1 + 0.2309699287 * lms2,
        -1.2684380041 * lms0 + 2.6097574007 * lms1 - 0.3413193963 * lms2,
        -0.0041960865 * lms0 - 0.7034186145 * lms1 + 1.7076147009 * lms2];
    };
    const rad = h * Math.PI / 180;
    const inGamut = rgb => rgb.every(v => v >= -0.001 && v <= 1.001);
    let chroma = c;
    let rgb = toLinear(l, chroma * Math.cos(rad), chroma * Math.sin(rad));
    if (!inGamut(rgb)) {
      let lo = 0;
      let hi = c;
      for (let i = 0; i < 24; i++) {
        chroma = (lo + hi) / 2;
        rgb = toLinear(l, chroma * Math.cos(rad), chroma * Math.sin(rad));
        if (inGamut(rgb))
          lo = chroma;
        else
          hi = chroma;
      }
      rgb = toLinear(l, lo * Math.cos(rad), lo * Math.sin(rad));
    }
    return Qt.rgba(srgbChannel(rgb[0]), srgbChannel(rgb[1]), srgbChannel(rgb[2]), 1);
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
