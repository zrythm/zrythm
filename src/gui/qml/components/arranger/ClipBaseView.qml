// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
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
  readonly property alias clipContent: clipContent.data
  readonly property alias clipContentContainer: clipContent
  required property ClipEditor clipEditor   // FIXME: is this needed?
  readonly property Clip clipObject: arrangerObject as Clip
  readonly property real contentHeight: clipContentContainer.height
  readonly property real contentWidth: clipContentContainer.width
  readonly property real effectiveReferenceWidth: referenceWidth > 0 ? referenceWidth : contentWidth
  // Clip surface colors derived from the unadjusted clip/track color
  // in one pass (DESIGN.md "Clips")
  readonly property var _clipColors: ZrythmTheme.clipColors(root.objectBaseColor, root._isMuted)
  // States tint the body toward the mode's polarity ink at fixed
  // ratios, giving identical gain on every hue; selection and
  // hover/press stack (DESIGN.md "Clips")
  readonly property color clipBodyColor: {
    const tint = (root.isSelected ? 0.20 : 0) + (root.down ? 0.26 : root.hovered ? 0.17 : 0);
    if (tint === 0)
      return _clipColors.body;
    return Qt.tint(_clipColors.body, ZrythmTheme.darkMode ? Qt.rgba(1, 1, 1, tint) : Qt.rgba(0, 0, 0, tint));
  }
  readonly property color clipBorderColor: _clipColors.border
  readonly property color clipScrimColor: _clipColors.scrim
  readonly property color contentInkColor: _clipColors.ink
  // Muted clips render fully achromatic and fade to half opacity
  // (DESIGN.md "Clips")
  opacity: root._isMuted ? ZrythmTheme.mutedClipOpacity : 1

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
  // When true, renderers extend loop content even if the clip isn't looped
  // (used during loop-resize of a non-looped clip).
  property bool loopPreview: false

  // Content density reference (constant during drag). 0 = use display width.
  property real referenceWidth: 0
  // Pixel offset for ResizeFromStart drag.
  property real referenceX: 0
  readonly property string regionName: arrangerObject.name.name
  readonly property real regionTicks: clipObject.timelineLengthTicks

  clip: true
  implicitHeight: 10
  implicitWidth: 10

  // Loop-point markers: dashed lines over the body and header,
  // beneath the name
  Loader {
    id: loopPointsLoader

    active: root.clipObject.looped || root.loopPreview
    anchors.fill: parent
    visible: active
    z: 3

    sourceComponent: Item {
      id: loopPointsContainer

      property var loopPoints: []

      function updateLoopPoints() {
        if (!root.clipObject.looped && !root.loopPreview) {
          loopPoints = [];
          return;
        }
        const regionLengthTicks = root.regionTicks;
        const displayEndTicks = root.effectiveReferenceWidth > 0 ? regionLengthTicks * (root.width / root.effectiveReferenceWidth) : regionLengthTicks;
        loopPoints = root.clipObject.loopPointTimelineTicks(displayEndTicks);
      }

      Component.onCompleted: updateLoopPoints()

      Connections {
        function onLoopablePropertiesChanged() {
          loopPointsContainer.updateLoopPoints();
        }

        function onTimelineLengthTicksChanged() {
          loopPointsContainer.updateLoopPoints();
        }

        target: root.clipObject
      }

      Connections {
        function onEffectiveReferenceWidthChanged() {
          loopPointsContainer.updateLoopPoints();
        }

        function onLoopPreviewChanged() {
          loopPointsContainer.updateLoopPoints();
        }

        function onWidthChanged() {
          loopPointsContainer.updateLoopPoints();
        }

        target: root
      }

      Repeater {
        model: loopPointsContainer.loopPoints

        delegate: LoopLine {
          required property real modelData

          x: (modelData / root.regionTicks) * root.effectiveReferenceWidth - root.referenceX
        }
      }
    }
  }

  Text {
    id: loopedIcon

    color: ZrythmTheme.polarityTextColor(root._clipColors.body)
    font.pixelSize: 10
    text: "⭯"
    visible: root.clipObject.looped && root.headerDetailLevel < 3
    z: 3

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
    z: 3

    anchors {
      right: loopedIcon.visible ? loopedIcon.left : parent.right
      rightMargin: loopedIcon.visible ? 4 : ZrythmTheme.buttonPadding / 2
      top: parent.top
      topMargin: ZrythmTheme.buttonPadding / 2
    }
  }

  // Clip body: the clip/track hue at the DESIGN.md "Clips" surface
  // recipe, drawn translucent so the lane grid reads through; content
  // draws above it in the zone below the header band
  Rectangle {
    id: bottomBackgroundRect

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    color: Qt.alpha(root.clipBodyColor, ZrythmTheme.bodyAlpha)
    radius: ZrythmTheme.toolButtonRadius
    z: 1
  }

  // Clip content is laid out in the zone below the header band, clear
  // of the name (DESIGN.md "Clips")
  Item {
    id: clipContent

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.topMargin: nameRect.height
    z: 2
  }

  // Header zone: a flat band in the scrim color holding the name and
  // header badges
  Rectangle {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    color: root.clipScrimColor
    height: nameRect.height
    topLeftRadius: ZrythmTheme.toolButtonRadius
    topRightRadius: ZrythmTheme.toolButtonRadius
    z: 2
  }

  // The 1 px recipe border rings the whole clip, drawn above the
  // header band so the outline stays unbroken
  Rectangle {
    anchors.fill: parent
    border.color: root.clipBorderColor
    border.width: 1
    color: "transparent"
    radius: ZrythmTheme.toolButtonRadius
    z: 3
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
      color: root.palette.windowText
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
      strokeColor: root.contentInkColor
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
