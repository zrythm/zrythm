// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmGui
import ZrythmStyle

ArrangerObjectBaseView {
  id: root

  required property ClipEditor clipEditor   // FIXME: is this needed?
  readonly property real contentHeight: regionContentContainer.height
  readonly property real contentWidth: regionContentContainer.width
  readonly property alias regionContent: regionContent.data
  readonly property alias regionContentContainer: regionContent
  readonly property string regionName: arrangerObject.name.name
  readonly property real regionTicks: arrangerObject.bounds.length.ticks

  clip: true
  implicitHeight: 10
  implicitWidth: 10

  Rectangle {
    id: topBackgroundRect

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    color: QmlUtils.adjustOpacity(QmlUtils.makeBrighter(root.objectColor, Style.darkMode ? 0.6 : 1.4), 0.7)
    height: nameRect.height
    topLeftRadius: Style.toolButtonRadius
    topRightRadius: Style.toolButtonRadius
    z: 3
  }

  Rectangle {
    id: bottomBackgroundRect

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    bottomLeftRadius: Style.toolButtonRadius
    bottomRightRadius: Style.toolButtonRadius
    color: root.objectColor
    height: parent.height - topBackgroundRect.height
    z: 1

    Item {
      id: regionContent

      anchors.fill: parent
    }
  }

  Rectangle {
    id: nameRect

    bottomRightRadius: Style.toolButtonRadius
    color: "transparent" // palette.button
    height: Math.min(textMetrics.height + 2 * (Style.buttonPadding / 2), root.height - Style.toolButtonRadius)
    topLeftRadius: Style.toolButtonRadius
    width: Math.min(textMetrics.width + 2 * Style.buttonPadding, root.width - Style.toolButtonRadius)
    z: 5

    anchors {
      left: parent.left
      top: parent.top
    }

    Text {
      id: nameText

      anchors.fill: parent
      bottomPadding: Style.buttonPadding / 2
      color: root.palette.buttonText
      elide: Text.ElideRight
      font: root.font
      horizontalAlignment: Text.AlignLeft
      leftPadding: Style.buttonPadding
      rightPadding: Style.buttonPadding
      text: root.arrangerObject.name.name
      topPadding: Style.buttonPadding / 2
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
          // FIXME: this should simply emit a signal and let the arranger handle the resize
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
          // FIXME: this should simply emit a signal and let the arranger handle the resize
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
