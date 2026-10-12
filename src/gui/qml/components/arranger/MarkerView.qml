// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmStyle
import Zrythm

ArrangerObjectBaseView {
  id: root

  colorRole: ArrangerObjectBaseView.ColorRole.Label

  readonly property int nameGap: 6
  readonly property real pennantHalfWidth: 6.5
  readonly property real barHalfWidth: 1.5
  readonly property int triangleWidth: 9
  // Start/end markers carry a directional glyph (|> / <|) whose bar
  // sits on the marker's position; custom markers a pennant whose
  // notch tip sits on it
  readonly property bool isStart: root.arrangerObject.isStartMarker ()
  readonly property bool isEnd: root.arrangerObject.isEndMarker ()
  // Head extent left/right of the anchor position
  readonly property real headLeft:
    root.isEnd ? -(root.barHalfWidth + root.triangleWidth)
    : root.isStart ? -root.barHalfWidth : -root.pennantHalfWidth
  readonly property real headRight:
    root.isStart ? root.barHalfWidth + root.triangleWidth
    : root.isEnd ? root.barHalfWidth : root.pennantHalfWidth
  // The item spans the whole glyph, so hover, drag, the eraser and
  // the selection outline cover everything visible; the parent
  // loader shifts the item by this offset from the marker's position
  readonly property real positionLeftOffset: root.headLeft
  // Where the marker's position sits within the item
  readonly property real anchorX: -root.headLeft
  // The type is the identity of the special markers, so they always
  // display their canonical names whatever a project file carries
  readonly property string displayName:
    root.isStart ? qsTr("Start") : root.isEnd ? qsTr("End") : root.arrangerObject.name.name

  height: textMetrics.height + 2 * ZrythmTheme.buttonPadding
  // The glyph may extend left of the anchor position; the name sits
  // in a quiet capsule to its right
  width: root.headRight - root.headLeft + root.nameGap
    + textMetrics.width + 2 * ZrythmTheme.buttonPadding

  // Head glyph in the content ink, anchored at the marker's position
  Canvas {
    id: head

    width: root.headRight - root.headLeft
    height: parent.height
    readonly property int notchDepth: 5

    onPaint: {
      const ctx = getContext("2d");
      ctx.clearRect(0, 0, width, height);
      ctx.fillStyle = root.objectAccentColor;
      const barX = root.anchorX;
      ctx.beginPath();
      if (root.isStart || root.isEnd) {
        ctx.rect(barX - root.barHalfWidth, 0, root.barHalfWidth * 2, height);
        const dir = root.isStart ? 1 : -1;
        const baseX = barX + dir * root.barHalfWidth;
        // A near-square glyph centered on the bar
        const triHeight = height * 0.55;
        const triTop = (height - triHeight) / 2;
        ctx.moveTo(baseX, triTop);
        ctx.lineTo(baseX + dir * root.triangleWidth, height / 2);
        ctx.lineTo(baseX, triTop + triHeight);
      } else {
        ctx.moveTo(0, 0);
        ctx.lineTo(width, 0);
        ctx.lineTo(width, height - notchDepth);
        ctx.lineTo(width / 2, height);
        ctx.lineTo(0, height - notchDepth);
      }
      ctx.closePath();
      ctx.fill();
    }
    Component.onCompleted: requestPaint()
    onHeightChanged: requestPaint()
    onWidthChanged: requestPaint()
  }
  onObjectAccentColorChanged: head.requestPaint()

  // Quiet capsule carrying the name, so the marker reads as one
  // object with a hover/click surface
  Rectangle {
    id: nameCapsule

    anchors.left: parent.left
    anchors.leftMargin: root.anchorX + root.headRight + root.nameGap
    anchors.verticalCenter: parent.verticalCenter
    width: textMetrics.width + 2 * ZrythmTheme.buttonPadding
    height: parent.height - 4
    radius: ZrythmTheme.toolButtonRadius
    color: Qt.alpha (root.objectColor, ZrythmTheme.bodyAlpha)
    border.color: root.objectBorderColor
    border.width: 1
  }

  Text {
    id: nameText

    anchors.centerIn: nameCapsule
    color: root.palette.text
    font: root.font
    text: root.displayName
  }

  TextMetrics {
    id: textMetrics

    font: nameText.font
    text: nameText.text
  }
}
