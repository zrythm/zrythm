// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm

Item {
  id: root

  property real balanceAtStart: 0.0
  required property ProcessorParameter balanceParameter
  property real balanceValue: balanceParameter.baseValue
  readonly property real defaultBalanceValue: 0.5
  property bool dragging: false
  property bool hovered: false
  property real lastX: 0
  property real lastY: 0
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

  Component.onCompleted: {
    mouseArea.acceptedButtons = Qt.LeftButton | Qt.RightButton;
  }

  ProcessorParameterOperator {
    id: paramOp

    Component.onCompleted: {
      if (root.balanceParameter && root.undoStack) {
        paramOp.processorParameter = root.balanceParameter;
        paramOp.undoStack = root.undoStack;
      }
    }
  }

  Rectangle {
    id: background

    anchors.fill: parent
    color: Style.backgroundColor
    opacity: root.hovered ? 0.8 : 0.6
    radius: 2
  }

  // Left channel indicator
  Rectangle {
    id: leftIndicator

    color: {
      var intensity = root.hovered || root.dragging ? 0.7 : 0.4;
      return Qt.rgba(0.0, 0.5, 0.5, intensity);
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
      return Qt.rgba(0.0, 0.5, 0.5, intensity);
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
      return Qt.rgba(0.5, 0.5, 0.5, intensity);
    }
    height: parent.height
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
      return Qt.rgba(0.0, 1.0, 0.0, intensity);
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

  MouseArea {
    id: mouseArea

    anchors.fill: parent
    hoverEnabled: true

    onDoubleClicked: {
      paramOp.setValue(root.defaultBalanceValue);
    }
    onEntered: {
      root.hovered = true;
    }
    onExited: {
      root.hovered = false;
    }
    onPositionChanged: function (mouse) {
      if (root.dragging) {
        var deltaX = mouse.x - root.lastX;
        var sensitivity = 0.002;

        // Lower sensitivity if Ctrl held
        if (mouse.modifiers & Qt.ControlModifier) {
          sensitivity = 0.001;
        }

        var newValue = Math.max(0, Math.min(1, root.balanceAtStart + deltaX * sensitivity));

        paramOp.setValue(newValue);
      }
    }
    onPressed: function (mouse) {
      root.dragging = true;
      root.balanceAtStart = root.balanceParameter.baseValue;
      root.lastX = mouse.x;
      mouse.accepted = true;
    }
    onReleased: {
      root.dragging = false;
    }
    onWheel: {
      var step = wheel.modifiers & Qt.ControlModifier ? 0.01 : 0.05;
      var newValue = Math.max(0, Math.min(1, root.balanceValue + (wheel.angleDelta.x > 0 ? step : -step)));
      paramOp.setValue(newValue);
      wheel.accepted = true;
    }
  }
}
