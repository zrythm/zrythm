// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle 1.0

ToolBar {
  id: root

  property alias centerItems: centerSection.children
  property alias leftItems: leftSection.children
  property alias rightItems: rightSection.children
  property double verticalMargin: 2 // bottom and top margin

  background: Rectangle {
    color: palette.window
  }
  contentItem: RowLayout {
    spacing: 4

    RowLayout {
      id: leftSection

      Layout.alignment: Qt.AlignLeft
      Layout.bottomMargin: root.verticalMargin
      Layout.fillWidth: true
      Layout.topMargin: root.verticalMargin
    }

    RowLayout {
      id: centerSection

      Layout.alignment: Qt.AlignCenter
      Layout.bottomMargin: root.verticalMargin
      Layout.topMargin: root.verticalMargin
    }

    RowLayout {
      id: rightSection

      Layout.alignment: Qt.AlignRight
      Layout.bottomMargin: root.verticalMargin
      Layout.topMargin: root.verticalMargin
    }
  }
}
