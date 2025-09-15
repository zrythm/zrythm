// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmGui
import ZrythmStyle

ArrangerObjectBaseView {
  id: root

  required property ClipEditor clipEditor
  property string regionName: arrangerObject.name.name

  implicitHeight: 10
  implicitWidth: 10

  Rectangle {
    id: backgroundRect

    anchors.fill: parent
    color: root.objectColor
    radius: Style.toolButtonRadius
  }

  Rectangle {
    id: nameRect

    bottomRightRadius: Style.toolButtonRadius
    color: root.palette.button
    height: Math.min(textMetrics.height + 2 * Style.buttonPadding, root.height - Style.toolButtonRadius)
    topLeftRadius: Style.toolButtonRadius
    width: Math.min(textMetrics.width + 2 * Style.buttonPadding, root.width - Style.toolButtonRadius)

    anchors {
      left: parent.left
      top: parent.top
    }

    Text {
      id: nameText

      anchors.fill: parent
      color: root.palette.buttonText
      elide: Text.ElideRight
      font: root.font
      horizontalAlignment: Text.AlignLeft
      padding: Style.buttonPadding
      text: root.arrangerObject.name.name
      verticalAlignment: Text.AlignVCenter
    }

    TextMetrics {
      id: textMetrics

      font: nameText.font
      text: nameText.text
    }
  }

  // Left resize handle
  Rectangle {
    id: leftHandle

    bottomLeftRadius: Style.toolButtonRadius
    color: Style.backgroundAppendColor
    height: parent.height
    topLeftRadius: Style.toolButtonRadius
    visible: root.hovered || leftDrag.drag.active
    width: 5
    z: 10

    MouseArea {
      id: leftDrag

      property real startX

      acceptedButtons: Qt.LeftButton
      anchors.fill: parent
      anchors.margins: -5
      cursorShape: Qt.SizeHorCursor
      drag.axis: Drag.XAxis
      drag.target: parent

      onPositionChanged: mouse => {
        if (drag.active) {
          let ticksDiff = (root.x - startX) / root.pxPerTick;
          root.arrangerObject.position.ticks += ticksDiff;
          if (root.arrangerObject.hasLength) {
            root.arrangerObject.clipStartTicks += ticksDiff;
            root.arrangerObject.endPosition.ticks -= ticksDiff;
          }
          startX = root.x;
          parent.x = 0;
        }
      }
      onPressed: mouse => {
        startX = root.x;
      }
    }
  }

  // Right resize handle
  Rectangle {
    id: rightHandle

    anchors.right: parent.right
    bottomRightRadius: Style.toolButtonRadius
    color: Style.backgroundAppendColor
    height: parent.height
    topRightRadius: Style.toolButtonRadius
    visible: root.hovered || rightDrag.drag.active
    width: 5
    z: 10

    MouseArea {
      id: rightDrag

      property real startX

      acceptedButtons: Qt.LeftButton
      anchors.fill: parent
      anchors.margins: -5
      cursorShape: Qt.SizeHorCursor
      drag.axis: Drag.XAxis
      drag.target: parent

      onPositionChanged: mouse => {
        if (drag.active) {
          let ticksDiff = (mouse.x - startX) / root.pxPerTick;
          root.arrangerObject.bounds.length.ticks += ticksDiff;
        }
      }
      onPressed: mouse => {
        startX = mouse.x;
      }
    }
  }
}
