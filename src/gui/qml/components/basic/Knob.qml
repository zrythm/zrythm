// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
* This file incorporates work covered by the following copyright and
* permission notice:
*
* ---
*
* SPDX-FileCopyrightText: Copyright (C) 2010 Paul Davis
* SPDX-License-Identifier: GPL-2.0-or-later
*
* Copyright (C) 2010 Paul Davis
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.
*
* ---
*/

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm

Control {
  id: root

  property bool arc: true
  property int bevel: 1
  property real defaultValue: 0.0

  // Interaction properties
  property bool dragging: false
  property bool flat: true
  property point lastCoordinates: Qt.point(0, 0)
  property real maxValue: 1.0
  property real minValue: 0.0
  property point cursorPosAtDragStart: Qt.point(0, 0)

  // Computed properties
  readonly property real normalizedValue: (value - minValue) / (maxValue - minValue)

  // Basic properties
  property int size: 30

  // Computed colors based on palette
  readonly property color darkArcBackgroundColor: Style.getColorBlendedTowardsContrast(palette.window)
  readonly property color colorAtZero: Style.getColorBlendedTowardsContrast(palette.text) //palette.mid
  readonly property color flatStyleColor: palette.button
  readonly property color flatTopColor: palette.highlight
  readonly property color hoverHighlightColor: Qt.rgba(1, 1, 1, 0.12)
  readonly property color knobBaseColor: palette.button
  readonly property color knobBorderColor: Qt.color("transparent") // palette.mid
  readonly property color pointerLineColor: palette.dark
  readonly property color pointerShadowColor: palette.shadow
  readonly property color colorAtMax: palette.accent

  onKnobBaseColorChanged: knobCanvas.requestPaint()

  // Unit for display
  property int unit: 0 // 0: none, 1: Hz, 2: MHz, 3: dB, 4: degrees, 5: seconds, 6: ms, 7: μs

  // Value properties
  property real value: 0.0
  property real valueAtStart: 0.0
  property real zero: 0.0

  // Signals
  signal bindMidiCC
  signal resetKnob

  implicitHeight: size
  implicitWidth: size

  ContextMenu.menu: Menu {
    id: contextMenu

    MenuItem {
      text: qsTr("Reset")

      onTriggered: {
        root.value = root.defaultValue;
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

  // Update canvas when value changes
  onValueChanged: {
    knobCanvas.requestPaint();
  }

  // Canvas for drawing the knob
  Canvas {
    id: knobCanvas

    anchors.fill: parent
    antialiasing: true

    onPaint: {
      var ctx = getContext("2d");
      ctx.reset();

      var width = parent.width;
      var height = parent.height;
      var scale = Math.min(width, height);
      var centerX = width / 2;
      var centerY = height / 2;

      // Calculate angle based on normalized value (0-1)
      var startAngle = (180 - 60) * Math.PI / 180; // 120 degrees in radians
      var endAngle = (360 + 60) * Math.PI / 180;   // 420 degrees in radians
      var valueAngle = startAngle + root.normalizedValue * (endAngle - startAngle);
      var zeroNormalized = (root.zero - root.minValue) / (root.maxValue - root.minValue);
      var zeroAngle = startAngle + zeroNormalized * (endAngle - startAngle);

      // Draw arc if enabled
      if (root.arc) {
        var innerRadius = scale * 0.38;
        var outerRadius = scale * 0.48;
        var progressWidth = outerRadius - innerRadius;
        var progressRadius = innerRadius + progressWidth / 2;

        // Dark arc background
        ctx.beginPath();
        ctx.arc(centerX, centerY, progressRadius, startAngle, endAngle);
        ctx.lineWidth = progressWidth;
        ctx.strokeStyle = root.darkArcBackgroundColor;
        ctx.stroke();

        // Colored arc based on value intensity
        var intensity = Math.abs(root.normalizedValue - zeroNormalized) / Math.max(zeroNormalized, 1.0 - zeroNormalized);
        var intensityInv = 1.0 - intensity;
        var r = intensityInv * root.colorAtZero.r + intensity * root.colorAtMax.r;
        var g = intensityInv * root.colorAtZero.g + intensity * root.colorAtMax.g;
        var b = intensityInv * root.colorAtZero.b + intensity * root.colorAtMax.b;

        ctx.beginPath();
        if (zeroAngle > valueAngle) {
          ctx.arc(centerX, centerY, progressRadius, valueAngle, zeroAngle);
        } else {
          ctx.arc(centerX, centerY, progressRadius, zeroAngle, valueAngle);
        }
        ctx.lineWidth = progressWidth;
        ctx.strokeStyle = Qt.rgba(r, g, b, 1.0);
        ctx.stroke();
      }

      // Draw knob body
      var centerRadius = root.arc ? scale * 0.33 : scale * 0.48;

      if (!root.flat) {
        // Knob shadow
        ctx.beginPath();
        ctx.arc(centerX + 1, centerY + 1, centerRadius - 1, 0, 2 * Math.PI);
        ctx.fillStyle = "rgba(0, 0, 0, 0.1)";
        ctx.fill();

        // Knob base
        ctx.beginPath();
        ctx.arc(centerX, centerY, centerRadius, 0, 2 * Math.PI);
        ctx.fillStyle = root.knobBaseColor;
        ctx.fill();

        if (root.bevel) {
          // Bevel effect
          var gradient = ctx.createLinearGradient(centerX, centerY - centerRadius, centerX, centerY + centerRadius);
          gradient.addColorStop(0, Qt.rgba(1, 1, 1, 0.2));
          gradient.addColorStop(0.2, Qt.rgba(1, 1, 1, 0.2));
          gradient.addColorStop(0.8, Qt.rgba(0, 0, 0, 0.2));
          gradient.addColorStop(1, Qt.rgba(0, 0, 0, 0.2));

          ctx.beginPath();
          ctx.arc(centerX, centerY, centerRadius, 0, 2 * Math.PI);
          ctx.fillStyle = gradient;
          ctx.fill();

          // Flat top
          ctx.beginPath();
          ctx.arc(centerX, centerY, centerRadius - 3, 0, 2 * Math.PI);
          ctx.fillStyle = root.flatTopColor;
          ctx.fill();
        } else {
          // Radial gradient
          var radialGradient = ctx.createRadialGradient(centerX - centerRadius, centerY - centerRadius, 1, centerX - centerRadius, centerY - centerRadius, centerRadius * 2.5);
          radialGradient.addColorStop(0, Qt.rgba(1, 1, 1, 0.2));
          radialGradient.addColorStop(1, Qt.rgba(0, 0, 0, 0.3));

          ctx.beginPath();
          ctx.arc(centerX, centerY, centerRadius, 0, 2 * Math.PI);
          ctx.fillStyle = radialGradient;
          ctx.fill();
        }
      } else {
        // Flat style
        ctx.beginPath();
        ctx.arc(centerX, centerY, centerRadius, 0, 2 * Math.PI);
        ctx.fillStyle = root.flatStyleColor;
        ctx.fill();
      }

      // Knob border
      ctx.beginPath();
      ctx.arc(centerX, centerY, centerRadius, 0, 2 * Math.PI);
      ctx.lineWidth = 0.8;
      ctx.strokeStyle = root.knobBorderColor;
      ctx.stroke();

      // Draw pointer line
      var pointerThickness = 3 * (scale / 80);
      var valueX = Math.cos(valueAngle);
      var valueY = Math.sin(valueAngle);

      if (!root.flat) {
        // Pointer shadow
        ctx.beginPath();
        ctx.moveTo(centerX + 1 + centerRadius * valueX, centerY + 1 + centerRadius * valueY);
        ctx.lineTo(centerX + 1 + centerRadius * 0.4 * valueX, centerY + 1 + centerRadius * 0.4 * valueY);
        ctx.lineWidth = pointerThickness;
        ctx.lineCap = "round";
        ctx.strokeStyle = root.pointerShadowColor;
        ctx.stroke();
      }

      // Pointer line
      ctx.beginPath();
      ctx.moveTo(centerX + centerRadius * valueX, centerY + centerRadius * valueY);
      ctx.lineTo(centerX + centerRadius * 0.4 * valueX, centerY + centerRadius * 0.4 * valueY);
      ctx.lineWidth = pointerThickness;
      ctx.lineCap = "round";
      ctx.strokeStyle = root.pointerLineColor;
      ctx.stroke();

      // Hover highlight
      if (root.hovered) {
        ctx.beginPath();
        ctx.arc(centerX, centerY, centerRadius, 0, 2 * Math.PI);
        ctx.fillStyle = root.hoverHighlightColor;
        ctx.fill();
      }
    }
  }

  // Value display text
  Text {
    id: valueText

    anchors.centerIn: parent
    color: palette.buttonText
    font.pixelSize: 8
    text: {
      var realVal = root.value;

      switch (root.unit) {
      case 3 // dB
      :
        var db = 20 * Math.log10(realVal);
        if (db < -100)
          return "-∞ dB";
        return db.toFixed(1) + " dB";
      case 1 // Hz
      :
        if (realVal >= 1000)
          return (realVal / 1000).toFixed(1) + " kHz";
        return realVal.toFixed(0) + " Hz";
      case 2 // MHz
      :
        return realVal.toFixed(1) + " MHz";
      case 4 // Degrees
      :
        return realVal.toFixed(0) + "°";
      case 5 // Seconds
      :
        return realVal.toFixed(1) + " s";
      case 6 // ms
      :
        return realVal.toFixed(0) + " ms";
      case 7 // μs
      :
        return realVal.toFixed(0) + " μs";
      default:
        return realVal.toFixed(2);
      }
    }
    visible: root.hovered || root.dragging
  }

  MouseArea {
    id: mouseArea

    anchors.fill: parent
    hoverEnabled: true

    onDoubleClicked: {
      root.value = root.defaultValue;
      knobCanvas.requestPaint();
    }
    onEntered: {
      knobCanvas.requestPaint();
    }
    onExited: {
      if (!root.dragging) {
        knobCanvas.requestPaint();
      }
    }
    onPositionChanged: function (mouse) {
      if (root.dragging) {
        var deltaX = mouse.x - root.lastCoordinates.x;
        var deltaY = root.lastCoordinates.y - mouse.y; // Invert Y for natural dragging
        var useY = Math.abs(deltaY) > Math.abs(deltaX);
        var delta = useY ? deltaY : deltaX;

        var sensitivity = 0.004;
        var newValue = Math.max(root.minValue, Math.min(root.maxValue, root.valueAtStart + delta * sensitivity));

        root.value = newValue;
        knobCanvas.requestPaint();

        // FIXME: warping doesn't change the value as expected
        // root.lastCoordinates = root.cursorPosAtDragStart
        CursorManager.warpCursor(root.cursorPosAtDragStart);
      }
    }
    onPressed: function (mouse) {
      if (mouse.button === Qt.LeftButton) {
        root.dragging = true;
        root.valueAtStart = root.value;
        root.lastCoordinates = Qt.point(mouse.x, mouse.y);
        root.cursorPosAtDragStart = CursorManager.cursorPosition()
        mouse.accepted = true;
      }
    }
    onReleased: function (mouse) {
      if (mouse.button === Qt.LeftButton) {
        root.dragging = false;
        knobCanvas.requestPaint();
      }
    }
    onWheel: function (wheel) {
      var step = wheel.modifiers & Qt.ControlModifier ? 0.01 : 0.05;
      var stepSize = step * (root.maxValue - root.minValue);
      var newValue = Math.max(root.minValue, Math.min(root.maxValue, root.value + (wheel.angleDelta.y > 0 ? stepSize : -stepSize)));
      root.value = newValue;
      knobCanvas.requestPaint();
      wheel.accepted = true;
    }
  }
}
