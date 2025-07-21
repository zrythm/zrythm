// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle

Control {
  id: root

  required property var arrangerObject
  readonly property alias down: dragArea.pressed
  readonly property color objectColor: {
    let c = arrangerObject.hasColor ? arrangerObject.color : track.color;
    if (arrangerObject.selected)
      c = Style.getColorBlendedTowardsContrastByFactor(c, 1.1);

    return Style.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.visualFocus, root.down);
  }
  required property real pxPerTick
  required property var track

  signal arrangerObjectClicked

  focusPolicy: Qt.StrongFocus
  font: arrangerObject.selected ? Style.arrangerObjectBoldTextFont : Style.arrangerObjectTextFont
  height: track.height
  hoverEnabled: true
  width: 100
  x: arrangerObject.position.ticks * pxPerTick

  ContextMenu.menu: Menu {
    MenuItem {
      text: qsTr("Test")

      onTriggered: console.log("Clicked")
    }
  }

  MouseArea {
    id: dragArea

    property real startX

    acceptedButtons: Qt.AllButtons
    anchors.fill: parent
    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    drag.axis: Drag.XAxis
    drag.target: parent
    drag.threshold: 0

    onPositionChanged: mouse => {
      if (drag.active) {
        if (mouse.button == Qt.LeftButton) {
          let newTicks = root.x / root.pxPerTick;
          let arrangerObject = root.arrangerObject;
          let ticksDiff = newTicks - arrangerObject.position.ticks;
          arrangerObject.position.ticks = newTicks;
          if (arrangerObject.hasLength)
            arrangerObject.endPosition.ticks += ticksDiff;
        }
      }
    }
    onPressed: mouse => {
      startX = root.x;
      root.forceActiveFocus();
      root.arrangerObjectClicked();
    }
    onReleased: {}
  }
}
