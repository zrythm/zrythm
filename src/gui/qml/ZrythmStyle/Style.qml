// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// zrythm color variant (accent | accent lighter | accent darker):
// F79616 | FFA533 | D68A0C
// more prominent: FF9F00 | FFB300 | F79616

pragma Singleton

import QtQuick

QtObject {
  id: root

  readonly property real animationDuration: 200
  readonly property int animationEasingType: Easing.OutExpo
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
  readonly property Palette colorPalette: Palette {
    accent: root.primaryColor
    alternateBase: root.getColorBlendedTowardsContrast(root.buttonBackgroundColor)
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
  readonly property color dangerColor: "#D90368"
  property bool darkMode: Application.styleHints.colorScheme === Qt.Dark
  readonly property real disabledOpacityFactor: 0.7
  readonly property real downEnhancementFactor: lightenFactor // enhance things pressed down by 30%
  readonly property font fadedTextFont: ({
      "family": root.fontFamily,
      "pixelSize": 11,
      "weight": Font.Normal
    })
  property string fontFamily: notoSansFont.name
  readonly property real fontPointSize: 10
  readonly property real inactiveOpacityFactor: 0.85
  readonly property color jonquilYellowColor: "#FFD100"
  readonly property real lightenFactor: 1.3 // lighten things up 10%, mainly used for hovering but can be used for other things like making parts of the UI stand out from the background
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
  readonly property color soloGreenColor: "#009B86"
  property color textColor: darkMode ? "#E3E3E3" : "#161616" // used in contrast with pageColor
  readonly property real textFieldRadius: 4
  readonly property real toolButtonRadius: 6
  readonly property int toolTipDelay: 700
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
    return root.isColorDark(arg) ? arg.darker(root.downEnhancementFactor) : arg.lighter(root.downEnhancementFactor);
  }

  function isColorDark(arg: color): bool {
    return arg.hslLightness < 0.5;
  }

  Component.onCompleted: {}
}
