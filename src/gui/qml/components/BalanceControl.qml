// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm

Item {
  id: root

  required property ProcessorParameter balanceParameter
  property real balanceValue: balanceParameter.baseValue
  readonly property real defaultBalanceValue: balanceParameter?.range.defaultNormalizedValue ?? 0.5
  readonly property bool dragging: mouseArea.dragging
  property bool hovered: false
  required property UndoStack undoStack

  signal bindMidiCC
  signal resetBalance

  implicitHeight: 16
  implicitWidth: 48

  ContextMenu.menu: Menu {
    id: contextMenu

    MenuItem {
      text: qsTr("Reset")

      onTriggered: {
        paramOp.setValue(root.defaultBalanceValue);
      }
    }

    MenuItem {
      text: qsTr("Bind MIDI CC")

      onTriggered: {
        root.bindMidiCC();
      }
    }
  }

  ProcessorParameterOperator {
    id: paramOp
  }

  Binding {
    property: "processorParameter"
    restoreMode: Binding.RestoreNone
    target: paramOp
    value: root.balanceParameter
    when: root.balanceParameter !== null
  }

  Binding {
    property: "undoStack"
    restoreMode: Binding.RestoreNone
    target: paramOp
    value: root.undoStack
    when: root.undoStack !== null
  }

  Rectangle {
    id: background

    anchors.fill: parent
    color: ZrythmTheme.getColorBlendedTowardsContrast(palette.window)
    opacity: root.hovered ? 0.8 : 0.6
    radius: ZrythmTheme.textFieldRadius
  }

  // Left channel indicator
  Rectangle {
    id: leftIndicator

    color: {
      var intensity = root.hovered || root.dragging ? 0.7 : 0.4;
      return palette.accent.alpha(intensity);
    }
    height: parent.height
    visible: root.balanceValue < 0.5
    width: {
      var halfWidth = parent.width / 2;
      var valuePx = root.balanceValue * parent.width;
      return root.balanceValue < 0.5 ? halfWidth - valuePx : 0;
    }

    anchors {
      right: parent.horizontalCenter
      top: parent.top
    }
  }

  // Right channel indicator
  Rectangle {
    id: rightIndicator

    color: {
      var intensity = root.hovered || root.dragging ? 0.7 : 0.4;
      return palette.accent.alpha(intensity);
    }
    height: parent.height
    visible: root.balanceValue > 0.5
    width: {
      var halfWidth = parent.width / 2;
      var valuePx = root.balanceValue * parent.width;
      return root.balanceValue > 0.5 ? valuePx - halfWidth : 0;
    }

    anchors {
      left: parent.horizontalCenter
      top: parent.top
    }
  }

  // Center line
  Rectangle {
    id: centerLine

    color: {
      var intensity = root.hovered || root.dragging ? 1.0 : 0.7;
      return palette.text.alpha(intensity);
    }
    height: parent.height
    visible: false
    width: 1

    anchors {
      horizontalCenter: parent.horizontalCenter
      top: parent.top
    }
  }

  // Current value indicator
  Rectangle {
    id: valueIndicator

    color: {
      var intensity = root.hovered || root.dragging ? 1.0 : 0.7;
      return palette.accent.alpha(intensity);
    }
    height: parent.height
    width: 2

    anchors {
      left: parent.left
      leftMargin: root.balanceValue * parent.width - 1
      top: parent.top
    }
  }

  // Left label
  Text {
    id: leftLabel

    color: palette.text
    font.pixelSize: 10
    text: "L"

    anchors {
      left: parent.left
      leftMargin: 3
      verticalCenter: parent.verticalCenter
    }
  }

  // Right label
  Text {
    id: rightLabel

    color: palette.text
    font.pixelSize: 10
    text: "R"

    anchors {
      right: parent.right
      rightMargin: 3
      verticalCenter: parent.verticalCenter
    }
  }

  // Value display
  Text {
    id: valueText

    anchors.centerIn: parent
    color: palette.text
    font.pixelSize: 10
    text: {
      var panVal = root.balanceValue - 0.5;
      var percentage = Math.abs(panVal) / 0.5 * 100;
      var sign = panVal < 0 ? "-" : "";
      return sign + Math.round(percentage) + "%";
    }
    visible: root.hovered || root.dragging
  }

  WarpDragArea {
    id: mouseArea

    allowedAxes: Qt.Horizontal
    anchors.fill: parent
    defaultValue: root.defaultBalanceValue
    value: root.balanceParameter.baseValue
    wheelAxis: Qt.Horizontal

    onEntered: {
      root.hovered = true;
    }
    onExited: {
      root.hovered = false;
    }
    onValueModified: function (newValue) {
      paramOp.setValue(newValue);
    }
  }
}
