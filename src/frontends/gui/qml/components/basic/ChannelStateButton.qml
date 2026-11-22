// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import ZrythmStyle

// Dense 18x18 checkable chip for channel state controls (mute, solo,
// record, listen and secondary channel toggles such as mono and phase).
// At rest the chip takes the button fill; ghost chips are transparent
// with dimmed ink until hovered, and materialize (take the button fill)
// when set while any sibling chip in their pill is checked. The checked
// state takes `checkedFill` with the polarity ink (DESIGN.md "Channel
// state buttons"). Chips are flat; grouping into pills is done by
// LinkedButtons, which flattens the shared corners.
Button {
  id: control

  // Fill used for the checked state; the content ink is chosen by
  // contrast against it
  property color checkedFill: control.palette.accent
  // Ghost chips are transparent at rest with dimmed ink
  property bool ghost: false
  // Letter/glyph size (the record dot renders larger than the letters)
  property int glyphPixelSize: ZrythmTheme.channelStateTextFont.pixelSize
  // Ink at rest for filled chips (the record dot uses the record tint)
  property color restInk: control.palette.buttonText
  // Small corner dot marking the monitor AUTO state
  property bool showActiveDot: false

  checkable: true
  font.bold: true
  font.family: ZrythmTheme.fontFamily
  font.pixelSize: control.glyphPixelSize
  hoverEnabled: true
  layer.enabled: false
  padding: 0
  horizontalPadding: 0

  // Ink for the label/glyph in the current state
  function inkColor(): color {
    if (control.checked)
      return ZrythmTheme.polarityTextColor(control.checkedFill);
    if (control.ghost)
      return Qt.alpha(control.palette.windowText, 0.62);
    return control.restInk;
  }

  icon {
    color: control.inkColor()
    height: 12
    width: 12

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }
  }

  background: Rectangle {
    implicitHeight: ZrythmTheme.compactControlHeight
    implicitWidth: ZrythmTheme.compactControlHeight
    radius: ZrythmTheme.textFieldRadius
    color: {
      if (control.ghost && !control.checked && (control.hovered || control.down))
        return ZrythmTheme.buttonHoverBackgroundAppendColor;
      const baseColor = control.checked ? control.checkedFill : control.ghost ? "transparent" : control.palette.button;
      return ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(baseColor, control.hovered, control.visualFocus, control.down);
    }
    border.color: control.palette.highlight
    border.width: control.visualFocus || control.down ? 2 : 0

    Behavior on border.width {
      animation: ZrythmTheme.propertyAnimation
    }

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }
  }

  contentItem: Item {
    IconLabel {
      anchors.centerIn: parent
      color: control.inkColor()
      display: control.display
      font: control.font
      icon: control.icon
      spacing: control.spacing
      text: control.text

      Behavior on color {
        animation: ZrythmTheme.propertyAnimation
      }
    }

    Rectangle {
      anchors.margins: 1
      anchors.right: parent.right
      anchors.top: parent.top
      color: control.palette.highlight
      height: 5
      radius: 2.5
      visible: control.showActiveDot
      width: 5
    }
  }
}
