// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle

// Thin bar shown at the bottom of the project window. Status information is
// added declaratively: assign items to leftItems/rightItems (e.g.
// StatusBarText or any other control); invisible items are skipped by the
// layout, so items can bind visible to a condition.
ToolBar {
  id: root

  property alias leftItems: leftSection.children
  property alias rightItems: rightSection.children

  implicitHeight: 24
  leftPadding: ZrythmTheme.buttonPadding * 2
  rightPadding: ZrythmTheme.buttonPadding * 2

  background: Rectangle {
    color: palette.window

    Rectangle {
      anchors {
        left: parent.left
        right: parent.right
        top: parent.top
      }
      color: Qt.alpha(palette.mid, 0.6)
      height: 1
    }
  }
  contentItem: RowLayout {
    spacing: ZrythmTheme.buttonPadding * 2

    RowLayout {
      id: leftSection

      Layout.alignment: Qt.AlignLeft
      Layout.fillWidth: true
      spacing: ZrythmTheme.buttonPadding * 3
    }

    RowLayout {
      id: rightSection

      Layout.alignment: Qt.AlignRight
      spacing: ZrythmTheme.buttonPadding * 3
    }
  }
}
