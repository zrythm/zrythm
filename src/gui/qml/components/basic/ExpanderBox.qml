// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick.Layouts
import QtQuick
import QtQuick.Controls
import ZrythmStyle

Control {
  id: root

  property bool expanded: true
  property alias frameContentItem: frame.contentItem
  property alias icon: expandButton.icon
  readonly property int radius: 4
  property string title: "Expander"
  property bool vertical: false

  padding: 4

  contentItem: ColumnLayout {
    spacing: 0

    Button {
      id: expandButton

      readonly property int iconSize: 16

      Layout.fillHeight: root.vertical
      Layout.fillWidth: !root.vertical
      hoverEnabled: true
      icon.height: iconSize
      icon.width: iconSize
      text: root.title

      background: ButtonBackgroundRect {
        bottomLeftRadius: root.expanded ? 0 : root.radius
        bottomRightRadius: root.expanded ? 0 : root.radius
        control: expandButton
        topLeftRadius: root.radius
        topRightRadius: root.radius
      }

      onClicked: root.expanded = !root.expanded
    }

    Frame {
      id: frame

      Layout.fillHeight: root.vertical
      Layout.fillWidth: !root.vertical
      visible: root.expanded

      Behavior on Layout.preferredHeight {
        animation: Style.propertyAnimation
      }
      background: Rectangle {
        anchors.fill: parent
        bottomLeftRadius: root.radius
        bottomRightRadius: root.radius
        color.a: 0 // invisible

        border {
          color: palette.button
          width: 2
        }
      }
    }

    Binding {
      property: "Layout.preferredHeight"
      target: frame
      value: 0
      when: !root.expanded
    }
  }
}
