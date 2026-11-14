// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm

Item {
  id: root

  property real currentAutomatedValue: faderValue
  property real currentModulatedValue: faderValue
  // dB stops drawn as ticks inside the groove and labels beside it
  readonly property list<int> dbStops: [6, 0, -6, -12, -24, -48]
  readonly property real defaultFaderValue: faderGain?.range.defaultNormalizedValue ?? 0.8
  readonly property string dbText: {
    const db = 20 * Math.log10(root.faderGain.range.convertFrom0To1(root.faderGain.baseValue));
    if (db < -100)
      return "-\u221E dB";
    return db.toFixed(1) + " dB";
  }
  readonly property bool dragging: mouseArea.dragging
  required property ProcessorParameter faderGain
  property real faderValue: faderGain.baseValue
  readonly property int handleHeight: 12
  property bool hovered: false
  // Width of the dB label column drawn beside the groove
  readonly property int legendWidth: 14
  required property ProcessorParameterOperator parameterOperator
  // Draws the dB scale labels beside the groove (mixer strips and the
  // inspector's fader section)
  property bool showScale: false

  signal bindMidiCC
  signal resetFader

  // Where a dB level sits on the travel, through the parameter's own
  // fader-curve inverse, so ticks land exactly where the cap reads.
  // The amplitude range tops at 2.0 (+6 dB), so +6 dB is the top stop.
  function posForDb(db: real): real {
    return faderGain.range.convertTo0To1(Math.pow(10, db / 20));
  }

  implicitHeight: 200
  implicitWidth: showScale ? legendWidth + 4 + 36 : 36

  ContextMenu.menu: Menu {
    id: contextMenu

    MenuItem {
      text: qsTr("Reset")

      onTriggered: {
        root.parameterOperator.setValue(root.faderGain, root.defaultFaderValue);
      }
    }

    MenuItem {
      text: qsTr("Bind MIDI CC")

      onTriggered: {
        root.bindMidiCC();
      }
    }
  }

  // Right-click opens the menu directly, matching the track and
  // plugin-slot context menus
  TapHandler {
    acceptedButtons: Qt.RightButton

    onTapped: contextMenu.popup()
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

  // Groove the cap travels in
  Rectangle {
    id: groove

    anchors {
      bottom: parent.bottom
      left: parent.left
      leftMargin: (root.showScale ? root.legendWidth + 4 : 0) + 7
      right: parent.right
      rightMargin: 7
      top: parent.top
    }
    border.color: Qt.alpha(palette.windowText, 0.1)
    border.width: 1
    color: ZrythmTheme.backgroundColor
    radius: ZrythmTheme.textFieldRadius
  }

  // dB scale labels beside the groove
  Item {
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: groove.left
    anchors.rightMargin: 4
    anchors.top: parent.top
    visible: root.showScale

    Repeater {
      model: root.dbStops

      Label {
        required property int modelData

        readonly property real tickPos: root.posForDb(modelData)

        anchors.right: parent.right
        color: modelData === 0 ? Qt.alpha(palette.accent, 0.8) : Qt.alpha(palette.windowText, 0.4)
        font: ZrythmTheme.xxSmallTextFont
        text: modelData > 0 ? "+" + modelData : String(modelData)
        y: Math.min(Math.max(parent.height * (1 - tickPos) - height / 2, 0), parent.height - height)
      }
    }
  }

  // dB scale ticks; 0 dB (unity) carries the accent
  Repeater {
    model: root.dbStops

    Rectangle {
      required property int modelData

      anchors.horizontalCenter: groove.horizontalCenter
      color: modelData === 0 ? Qt.alpha(palette.accent, 0.55) : Qt.alpha(palette.windowText, 0.12)
      height: 1
      width: modelData === 0 ? groove.width : groove.width - 8
      y: parent.height * (1 - root.posForDb(modelData))
    }
  }

  // Thin marker for the value automation/modulation currently produces
  Rectangle {
    anchors {
      bottom: groove.bottom
      right: groove.right
    }
    color: QmlUtils.adjustOpacity(palette.highlightedText, 0.8)
    height: groove.height * root.currentModulatedValue
    visible: root.currentModulatedValue !== root.faderValue
    width: 2
  }

  Rectangle {
    id: handle

    color: ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(palette.dark, root.hovered, false, root.dragging)
    height: root.handleHeight
    radius: 3
    width: 30

    anchors {
      horizontalCenter: groove.horizontalCenter
    }
    y: Math.min(Math.max(parent.height * (1 - root.faderValue) - height / 2, 0), parent.height - height)

    Rectangle {
      anchors.centerIn: parent
      color: root.palette.window
      height: 2
      radius: 1
      width: 16
    }

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }
  }

  WarpDragArea {
    id: mouseArea

    allowedAxes: Qt.Vertical
    anchors.fill: parent
    defaultValue: root.defaultFaderValue
    value: root.faderGain.baseValue

    onEntered: {
      root.hovered = true;
    }
    onExited: {
      root.hovered = false;
    }
    onValueModified: function (newValue) {
      root.parameterOperator.setValue(root.faderGain, newValue);
    }
  }
}
