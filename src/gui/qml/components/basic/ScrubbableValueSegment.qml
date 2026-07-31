// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle

/**
 * Text segment whose value can be adjusted by dragging or scrolling.
 *
 * Shows a subtle monochrome highlight while hovered or dragged. Value editing
 * is delegated to an internal WarpDragArea: bind value and react to
 * valueModified. Set referenceText to reserve width (e.g. "99") so the
 * segment does not resize while its text changes.
 */
Control {
  id: root

  property alias allowedAxes: dragArea.allowedAxes
  readonly property bool dragging: dragArea.dragging
  property alias from: dragArea.from
  property alias hoverCursorShape: dragArea.hoverCursorShape
  property alias pixelsForFullRange: dragArea.pixelsForFullRange

  /// Text whose width is reserved even when the current text is narrower
  property string referenceText: ""
  required property string text
  property alias to: dragArea.to
  property alias value: dragArea.value
  property alias wheelFineStep: dragArea.wheelFineStep
  property alias wheelStep: dragArea.wheelStep

  /// Emitted when a drag ends, either by release or cancellation
  signal dragEnded

  /// Emitted when a drag starts
  signal dragStarted

  /// Emitted when the drag or wheel requests a new value
  signal valueModified(real newValue)

  padding: 0

  contentItem: Item {
    implicitHeight: textItem.implicitHeight
    // Reserve the reference text's width plus slack so the segment never
    // resizes, regardless of the font's digit widths
    implicitWidth: Math.max(textItem.implicitWidth, referenceTextMetrics.advanceWidth + 2)

    Rectangle {
      anchors.fill: parent
      color: Qt.alpha(root.palette.text, 0.15)
      radius: 3
      visible: dragArea.containsMouse || dragArea.dragging
    }

    Text {
      id: textItem

      anchors.centerIn: parent
      color: root.palette.text
      font: ZrythmTheme.semiBoldTextFont
      text: root.text
    }

    TextMetrics {
      id: referenceTextMetrics

      font: textItem.font
      text: root.referenceText
    }

    WarpDragArea {
      id: dragArea

      anchors.fill: parent
      resetOnDoubleClick: false

      onDragEnded: root.dragEnded()
      onDragStarted: root.dragStarted()
      onValueModified: function (newValue) {
        root.valueModified(newValue);
      }
    }
  }
}
