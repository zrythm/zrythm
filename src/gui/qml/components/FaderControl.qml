// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm

Item {
  id: root

  property real ampAtStart: 0.0
  readonly property int bgRadius: Style.toolButtonRadius
  property real currentAutomatedValue: faderValue
  property real currentModulatedValue: faderValue
  readonly property real defaultFaderValue: 0.8
  property bool dragging: false
  required property ProcessorParameter faderGain
  property real faderValue: faderGain.baseValue
  readonly property int handleHeight: 12
  property bool hovered: false
  property real lastX: 0
  property real lastY: 0
  required property UndoStack undoStack

  signal bindMidiCC
  signal resetFader

  implicitHeight: 200
  implicitWidth: 36

  ContextMenu.menu: Menu {
    id: contextMenu

    MenuItem {
      text: qsTr("Reset")

      onTriggered: {
        paramOp.setValue(root.defaultFaderValue);
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

  // Update currently automated/modulated values
  Timer {
    interval: 16 // ~60fps
    repeat: true
    running: root.faderGain !== null

    onTriggered: {
      root.currentAutomatedValue = root.faderGain.valueAfterAutomationApplied();
      root.currentModulatedValue = root.faderGain.currentValue();
    }
  }

  ProcessorParameterOperator {
    id: paramOp

    Component.onCompleted: {
      if (root.faderGain && root.undoStack) {
        paramOp.processorParameter = root.faderGain;
        paramOp.undoStack = root.undoStack;
      }
    }
  }

  Rectangle {
    id: background

    anchors.fill: parent
    color: Style.getColorBlendedTowardsContrast(palette.window)
    opacity: root.hovered ? 0.8 : 0.6
    radius: root.bgRadius
  }

  Rectangle {
    id: fillRect

    readonly property color endColor: Style.getColorBlendedTowardsContrast(palette.accent)

    bottomLeftRadius: root.bgRadius
    bottomRightRadius: root.bgRadius
    color: {
      const intensity = root.faderValue;
      const fgColor = palette.accent;

      const r = (1.0 - intensity) * endColor.r + intensity * fgColor.r;
      const g = (1.0 - intensity) * endColor.g + intensity * fgColor.g;
      const b = (1.0 - intensity) * endColor.b + intensity * fgColor.b;
      let a = (1.0 - intensity) * endColor.a + intensity * fgColor.a;

      if (!root.hovered) {
        a = 0.9;
      }

      return Qt.rgba(r, g, b, a);
    }
    height: parent.height * root.faderValue
    topLeftRadius: 2
    topRightRadius: 2

    anchors {
      bottom: parent.bottom
      left: parent.left
      margins: 3
      right: parent.right
    }
  }

  Rectangle {
    id: currentValueRect

    bottomRightRadius: root.bgRadius
    color: QmlUtils.adjustOpacity(palette.highlightedText, 0.8)
    height: parent.height * root.currentModulatedValue
    width: 3
    visible: root.currentModulatedValue != root.faderValue

    anchors {
      bottom: parent.bottom
      margins: 3
      right: parent.right
    }
  }

  Rectangle {
    id: handle

    color: {
      var brightness = root.hovered || root.dragging ? 0.84 : 0.72;
      return palette.button;  // Qt.rgba(brightness, brightness, brightness, 1.0);
    }
    height: root.handleHeight
    radius: 3
    width: parent.width

    anchors {
      bottom: fillRect.top
      bottomMargin: root.handleHeight / 2
      horizontalCenter: parent.horizontalCenter
    }
  }

  Text {
    id: valueText

    anchors.centerIn: handle
    color: palette.buttonText
    font.pixelSize: 10
    text: {
      var db = 20 * Math.log10(root.faderGain.baseValue);
      if (db < -100)
        return "-∞";
      return db.toFixed(1) + " dB";
    }
    visible: root.hovered || root.dragging
  }

  MouseArea {
    id: mouseArea

    anchors.fill: parent
    hoverEnabled: true

    onDoubleClicked: {
      paramOp.setValue(root.defaultFaderValue);
    }
    onEntered: {
      root.hovered = true;
    }
    onExited: {
      root.hovered = false;
    }
    onPositionChanged: function (mouse) {
      if (root.dragging) {
        var deltaY = root.lastY - mouse.y;
        var sensitivity = 0.005;
        var newValue = Math.max(0, Math.min(1, root.ampAtStart + deltaY * sensitivity));

        paramOp.setValue(newValue);
      }
    }
    onPressed: function (mouse) {
      root.dragging = true;
      root.ampAtStart = root.faderGain.baseValue;
      root.lastY = mouse.y;
      mouse.accepted = true;
    }
    onReleased: {
      root.dragging = false;
    }
    onWheel: {
      var step = wheel.modifiers & Qt.ControlModifier ? 0.01 : 0.05;
      var newValue = Math.max(0, Math.min(1, root.faderValue + (wheel.angleDelta.y > 0 ? step : -step)));
      paramOp.setValue(newValue);
      wheel.accepted = true;
    }
  }
}
