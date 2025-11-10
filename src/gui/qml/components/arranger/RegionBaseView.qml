// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmGui
import ZrythmStyle
import QtQuick.Controls.impl
import QtQuick.Shapes

ArrangerObjectBaseView {
  id: root

  required property ClipEditor clipEditor   // FIXME: is this needed?
  readonly property real contentHeight: regionContentContainer.height
  readonly property real contentWidth: regionContentContainer.width
  readonly property ArrangerObjectLoopRange loopRange: arrangerObject.loopRange
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

    IconImage {
      id: loopedIcon

      readonly property real iconSize: 14

      color: root.palette.buttonText
      fillMode: Image.PreserveAspectFit
      source: ResourceManager.getIconUrl("gnome-icon-library", "loop-arrow-symbolic.svg")
      sourceSize.height: iconSize
      sourceSize.width: iconSize
      visible: root.loopRange.looped

      anchors {
        right: parent.right
        rightMargin: Style.buttonPadding / 2
        top: parent.top
        topMargin: Style.buttonPadding / 2
      }
    }
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

    Loader {
      id: loopPointsLoader

      active: root.loopRange.looped
      anchors.fill: parent
      visible: active
      z: 100

      sourceComponent: Repeater {
        id: loopPoints

        model: {
          if (!root.loopRange.looped) {
            return []
          }

          const loopPoints = []

          // Calculate loop length manually
          const loopStartTicks = root.loopRange.loopStartPosition.ticks
          const loopEndTicks = root.loopRange.loopEndPosition.ticks
          const loopLengthTicks = Math.max(0.0, loopEndTicks - loopStartTicks)

          // If loop length is 0, no loop points
          if (loopLengthTicks <= 0) {
            return []
          }

          const clipStartTicks = root.loopRange.clipStartPosition.ticks
          const regionLengthTicks = root.regionTicks

          // Calculate the first loop point (relative to region start)
          const firstLoopPoint = loopEndTicks - clipStartTicks

          // Add loop points until we reach the end of the region
          let currentPoint = firstLoopPoint
          while (currentPoint < regionLengthTicks) {
            loopPoints.push(currentPoint)
            currentPoint += loopLengthTicks
          }

          return loopPoints
        }

        delegate: LoopLine {
          required property real modelData
          readonly property real loopPointTicks: modelData
          id: loopLine

          x: (loopPointTicks / root.regionTicks) * root.width
        }
      }
    }
  }

  Rectangle {
    id: nameRect

    bottomRightRadius: Style.toolButtonRadius
    color: "transparent" // palette.button
    height: Math.min(textMetrics.height + 2 * (Style.buttonPadding / 2), root.height - Style.toolButtonRadius)
    topLeftRadius: Style.toolButtonRadius
    z: 5

    anchors {
      left: parent.left
      right: parent.right
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

  component LoopLine: Shape {
    id: shape

    width: 2

    anchors {
      bottom: parent.bottom
      top: parent.top
    }

    ShapePath {
      id: shapePath

      dashPattern: [4, 4]
      fillColor: "transparent"
      startX: 0
      startY: 0
      strokeColor: root.palette.buttonText
      strokeStyle: ShapePath.DashLine
      strokeWidth: 1

      PathLine {
        x: 1
        y: 0
      }

      PathLine {
        x: 1
        y: shape.height
      }
    }
  }
}
