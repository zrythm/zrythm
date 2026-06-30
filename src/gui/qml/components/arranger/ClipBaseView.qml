// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmGui
import ZrythmStyle
import QtQuick.Shapes

ArrangerObjectBaseView {
  id: root

  readonly property real _rightInset: {
    let w = ZrythmTheme.buttonPadding / 2;
    if (loopedIcon.visible)
      w += loopedIcon.width + 4;
    if (headerExtraContainer.childrenRect.width > 0)
      w += headerExtraContainer.childrenRect.width + 4;
    return w;
  }
  required property ClipEditor clipEditor   // FIXME: is this needed?
  readonly property Clip clipObject: arrangerObject as Clip
  readonly property real contentHeight: clipContentContainer.height
  readonly property real contentWidth: clipContentContainer.width

  // Progressive header disclosure: 0=full badge+loop, 1=symbol badge+loop,
  // 2=loop only, 3=name only. Priority: name > loop > badge.
  readonly property int headerDetailLevel: {
    const minName = 24;
    const leftPad = ZrythmTheme.buttonPadding;
    const rightBase = ZrythmTheme.buttonPadding / 2;
    const decorAvail = root.width - minName - leftPad - rightBase;
    const fullBadge = 60;
    const symbolBadge = 14;
    const loopSpace = root.clipObject.looped ? 18 : 0;
    if (decorAvail >= fullBadge + loopSpace)
      return 0;
    if (decorAvail >= symbolBadge + loopSpace)
      return 1;
    if (decorAvail >= loopSpace)
      return 2;
    return 3;
  }
  readonly property alias headerExtra: headerExtraContainer.data
  readonly property alias clipContent: clipContent.data
  readonly property alias clipContentContainer: clipContent
  readonly property string regionName: arrangerObject.name.name
  readonly property real regionTicks: clipObject.timelineLengthTicks

  clip: true
  implicitHeight: 10
  implicitWidth: 10

  Rectangle {
    id: topBackgroundRect

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    color: QmlUtils.adjustOpacity(QmlUtils.makeBrighter(root.objectColor, ZrythmTheme.darkMode ? 0.6 : 1.4), 0.7)
    height: nameRect.height
    topLeftRadius: ZrythmTheme.toolButtonRadius
    topRightRadius: ZrythmTheme.toolButtonRadius
    z: 3

    Text {
      id: loopedIcon

      color: root.palette.buttonText
      font.pixelSize: 10
      text: "⭯"
      visible: root.clipObject.looped && root.headerDetailLevel < 3

      anchors {
        right: parent.right
        rightMargin: ZrythmTheme.buttonPadding / 2
        top: parent.top
        topMargin: ZrythmTheme.buttonPadding / 2
      }
    }

    Item {
      id: headerExtraContainer

      height: childrenRect.height
      visible: headerExtraContainer.children.length > 0
      width: childrenRect.width

      anchors {
        right: loopedIcon.visible ? loopedIcon.left : parent.right
        rightMargin: loopedIcon.visible ? 4 : ZrythmTheme.buttonPadding / 2
        top: parent.top
        topMargin: ZrythmTheme.buttonPadding / 2
      }
    }
  }

  Rectangle {
    id: bottomBackgroundRect

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    bottomLeftRadius: ZrythmTheme.toolButtonRadius
    bottomRightRadius: ZrythmTheme.toolButtonRadius
    color: root.objectColor
    height: parent.height - topBackgroundRect.height
    z: 1

    Item {
      id: clipContent

      anchors.fill: parent
    }

    Loader {
      id: loopPointsLoader

      active: root.clipObject.looped
      anchors.fill: parent
      visible: active
      z: 100

      sourceComponent: Repeater {
        id: loopPoints

        model: {
          if (!root.clipObject.looped) {
            return [];
          }

          const warp = root.arrangerObject.contentWarp;

          function toTimelineTicksRelative(ticks: real): real {
            return warp ? warp.contentToTimelineTicksRelative(ticks) : ticks;
          }

          const loopPoints = [];

          const loopStartTicks = toTimelineTicksRelative(root.clipObject.loopStartPosition.ticks);
          const loopEndTicks = toTimelineTicksRelative(root.clipObject.loopEndPosition.ticks);
          const loopLengthTicks = Math.max(0.0, loopEndTicks - loopStartTicks);

          // If loop length is 0, no loop points
          if (loopLengthTicks <= 0) {
            return [];
          }

          const clipStartTicks = toTimelineTicksRelative(root.clipObject.clipStartPosition.ticks);
          const regionLengthTicks = root.regionTicks;

          // Calculate the first loop point (relative to clip start)
          const firstLoopPoint = loopEndTicks - clipStartTicks;

          // Add loop points until we reach the end of the clip
          let currentPoint = firstLoopPoint;
          while (currentPoint < regionLengthTicks) {
            loopPoints.push(currentPoint);
            currentPoint += loopLengthTicks;
          }

          return loopPoints;
        }

        delegate: LoopLine {
          id: loopLine

          readonly property real loopPointTicks: modelData
          required property real modelData

          x: (loopPointTicks / root.regionTicks) * root.width
        }
      }
    }
  }

  Rectangle {
    id: nameRect

    bottomRightRadius: ZrythmTheme.toolButtonRadius
    color: "transparent" // palette.button
    height: Math.min(textMetrics.height + 2 * (ZrythmTheme.buttonPadding / 2), root.height - ZrythmTheme.toolButtonRadius)
    topLeftRadius: ZrythmTheme.toolButtonRadius
    z: 5

    anchors {
      left: parent.left
      right: parent.right
      top: parent.top
    }

    Text {
      id: nameText

      anchors.fill: parent
      bottomPadding: ZrythmTheme.buttonPadding / 2
      color: root.palette.buttonText
      elide: Text.ElideRight
      font: root.font
      horizontalAlignment: Text.AlignLeft
      leftPadding: ZrythmTheme.buttonPadding
      rightPadding: root._rightInset
      text: root.arrangerObject.name.name
      topPadding: ZrythmTheme.buttonPadding / 2
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
