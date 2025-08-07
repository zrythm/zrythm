// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle

// upward popup TODO
ColumnLayout {
  id: root

  property bool directionUpward: false
  property alias iconSource: mainButton.icon.source
  property alias menuItems: menuLoader.sourceComponent
  property alias menuTooltipText: arrowButtonTooltip.text
  property alias text: mainButton.text
  property alias tooltipText: mainButtonTooltip.text

  signal clicked

  spacing: 0

  LinkedButtons {
    id: rowLayout

    Layout.fillHeight: true
    Layout.fillWidth: true

    Button {
      id: mainButton

      onClicked: root.clicked()

      ToolTip {
        id: mainButtonTooltip

      }
    }

    Button {
      id: arrowButton

      Layout.preferredWidth: 16
      font.pointSize: 6
      leftPadding: 0
      rightPadding: 0
      text: root.directionUpward ? "▲" : "▼"

      onClicked: menuLoader.item.open()

      ToolTip {
        id: arrowButtonTooltip

        text: qsTr("More Options...")
      }
    }
  }

  Loader {
    id: menuLoader

    sourceComponent: Menu {
      Action {
        text: "Fill Me Fill Me Fill Me Fill Me Fill Me Fill Me Fill Me Fill Me Fill Me"
      }

      Action {
        text: "Fill Me2"
      }

      Action {
        text: "Fill Me3"
      }
    }
  }
}
