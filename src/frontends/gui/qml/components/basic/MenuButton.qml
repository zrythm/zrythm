// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle

/**
 * Toolbar button that opens a menu of choices instead of performing an
 * immediate action.
 *
 * The label is a noun naming what the menu contains (e.g. "Filters"),
 * followed by a trailing chevron-down glyph that signals the popup.
 * Clicking opens `menu`: a Menu opens at the click position, and any
 * other Popup opens at its own position. A null menu means clicking
 * does nothing. While the state the menu controls is in effect (e.g.
 * filters applied), `active` gives the label and chevron the accent
 * color. `tooltipText` is shown on hover.
 */
ToolButton {
  id: root

  property bool active: false
  property T.Popup menu: null
  property alias tooltipText: toolTip.text

  palette.buttonText: root.active ? ZrythmTheme.colorPalette.accent : ZrythmTheme.colorPalette.buttonText

  onClicked: {
    if (root.menu === null)
      return;
    if (root.menu instanceof Menu)
      (root.menu as Menu).popup();
    else
      root.menu.open();
  }

  contentItem: Row {
    layoutDirection: root.mirrored ? Qt.RightToLeft : Qt.LeftToRight
    spacing: root.spacing

    IconLabel {
      anchors.verticalCenter: parent.verticalCenter
      color: root.down ? ZrythmTheme.getStrongerColor(root.palette.buttonText) : root.palette.buttonText
      display: root.display
      font: root.font
      icon: root.icon
      mirrored: root.mirrored
      spacing: root.spacing
      text: root.text

      Behavior on color {
        animation: ZrythmTheme.propertyAnimation
      }
    }

    ColorImage {
      anchors.verticalCenter: parent.verticalCenter
      color: root.down ? ZrythmTheme.getStrongerColor(root.palette.buttonText) : root.palette.buttonText
      height: 12
      source: "qrc:/qt/qml/Zrythm/icons/lucide/chevron-down.svg"
      sourceSize.height: height
      sourceSize.width: width
      width: 12

      Behavior on color {
        animation: ZrythmTheme.propertyAnimation
      }
    }
  }

  ToolTip {
    id: toolTip

    visible: root.hovered && root.tooltipText.length > 0
  }
}
