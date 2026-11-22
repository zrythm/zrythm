// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle

/**
 * Horizontal amount bar: a colored fill grows from the left over a dim
 * base of the same color, with a label on top. The amount follows
 * horizontal drags and the mouse wheel. A press that releases without
 * modifying the value emits tapped() so the host can use a plain click
 * for another action (e.g. open a menu).
 */
Control {
  id: root

  /** Color of the fill; the base underneath is the same color dimmed */
  property color fillColor: palette.accent
  /** Current amount in the 0..1 range */
  property real amount: 0.0
  /** Text drawn over the bar */
  property string label: ""
  /** Value text drawn right-aligned while dragging (e.g. "-3.2 dB") */
  property string valueText: ""

  /** Emitted when a drag, wheel step or double-click requests a new amount */
  signal amountModified(real newAmount)
  /** Emitted when a drag gesture begins or ends; hosts forward these to
   * parameter gestures */
  signal dragEnded
  signal dragStarted
  /** Emitted when the bar is clicked without the value changing */
  signal tapped

  // A press that never modifies the value is a tap
  property bool tapCandidate: false

  padding: 0
  implicitHeight: 20

  background: Item {
    Rectangle {
      anchors.fill: parent
      color: Qt.alpha(
        root.fillColor,
        !root.enabled ? 0.13 : (dragArea.containsMouse || dragArea.dragging) ? 0.38 : 0.26)
      radius: ZrythmTheme.textFieldRadius
    }

    Rectangle {
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.top: parent.top
      color: Qt.alpha(
        root.fillColor,
        !root.enabled ? 0.4 : (dragArea.containsMouse || dragArea.dragging) ? 1.0 : 0.88)
      radius: ZrythmTheme.textFieldRadius
      width: parent.width * root.amount
    }
  }

  contentItem: RowLayout {
    spacing: 0

    Label {
      Layout.fillWidth: true
      elide: Text.ElideRight
      font: ZrythmTheme.smallTextFont
      leftPadding: 6
      text: root.label
    }

    Label {
      font: ZrythmTheme.smallTextFont
      rightPadding: 6
      text: root.valueText
      visible: dragArea.dragging && root.valueText !== ""
    }
  }

  WarpDragArea {
    id: dragArea

    allowedAxes: Qt.Horizontal
    anchors.fill: parent
    enabled: root.enabled
    resetOnDoubleClick: false
    value: root.amount
    wheelAxis: Qt.Horizontal

    onDragEnded: {
      if (root.tapCandidate)
        root.tapped();
      root.dragEnded();
    }
    onDragStarted: {
      root.tapCandidate = true;
      root.dragStarted();
    }
    onValueModified: function (newAmount) {
      root.tapCandidate = false;
      root.amountModified(newAmount);
    }
  }
}
