// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm

Control {
  id: root

  required property ArrangerObject arrangerObject
  required property bool isSelected
  // readonly property alias down: dragArea.pressed
  readonly property color objectColor: {
    let c = arrangerObject.hasColor ? arrangerObject.color : track.color;
    if (root.isSelected)
      c = Style.getColorBlendedTowardsContrastByFactor(c, 1.1);

    return Style.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.visualFocus, root.down);
  }
  required property real pxPerTick
  required property Track track

  signal selectionRequested(var mouse)

  function requestSelection(mouse: var) {
    selectionRequested(mouse);
  }

  focusPolicy: Qt.StrongFocus
  font: root.isSelected ? Style.arrangerObjectBoldTextFont : Style.arrangerObjectTextFont
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
    propagateComposedEvents: true

    onPositionChanged: mouse => {
      mouse.accepted = false;
    }
    onPressed: mouse => {
      mouse.accepted = false;
    }
  }
}
