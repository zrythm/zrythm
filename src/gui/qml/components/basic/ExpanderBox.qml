// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick.Layouts
import QtQuick
import QtQuick.Controls
import ZrythmStyle

ColumnLayout {
  id: root

  property var contentItem
  property alias icon: expandButton.icon
  property string title: "Expander"
  property bool vertical: false

  spacing: 0

  ItemDelegate {
    id: expandButton

    readonly property int iconSize: 16

    Layout.fillHeight: root.vertical
    Layout.fillWidth: !root.vertical
    checkable: true
    checked: true
    hoverEnabled: true
    icon.height: iconSize
    icon.width: iconSize
    text: root.title

    background: Rectangle {
      readonly property color baseColor: palette.button
      readonly property color colorAdjustedForHoverOrFocusOrDown: Style.adjustColorForHoverOrVisualFocusOrDown(baseColor, expandButton.hovered, expandButton.visualFocus, expandButton.down)

      anchors.fill: parent
      color: colorAdjustedForHoverOrFocusOrDown
      implicitHeight: Style.buttonHeight
      implicitWidth: 100
      radius: 4

      Behavior on color {
        animation: Style.propertyAnimation
      }
    }
  }

  Frame {
    id: frame

    Layout.fillHeight: root.vertical
    Layout.fillWidth: !root.vertical
    contentItem: root.contentItem
    visible: expandButton.checked

    Behavior on Layout.preferredHeight {
      animation: Style.propertyAnimation
    }
  }

  Binding {
    property: "Layout.preferredHeight"
    target: frame
    value: 0
    when: !expandButton.checked
  }
}
