// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
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

Control {
  id: root

  property bool arc: true
  property int bevel: 1
  readonly property color colorAtMax: palette.accent
  readonly property color colorAtZero: ZrythmTheme.getColorBlendedTowardsContrast(palette.text) //palette.mid
  readonly property color darkArcBackgroundColor: ZrythmTheme.getColorBlendedTowardsContrast(palette.window)
  property real defaultValue: 0.0
  readonly property bool dragging: mouseArea.dragging
  property bool flat: true
  readonly property color flatStyleColor: palette.button
  readonly property color flatTopColor: palette.highlight
  readonly property color hoverHighlightColor: Qt.rgba(1, 1, 1, 0.12)
  readonly property color knobBaseColor: palette.button
  readonly property color knobBorderColor: Qt.color("transparent") // palette.mid
  property real maxValue: 1.0
  property real minValue: 0.0
  readonly property real normalizedValue: (value - minValue) / (maxValue - minValue)
  readonly property color pointerLineColor: palette.dark
  readonly property color pointerShadowColor: palette.shadow
  property int size: 30
  property int unit: 0 // 0: none, 1: Hz, 2: MHz, 3: dB, 4: degrees, 5: seconds, 6: ms, 7: μs
  property real value: 0.0
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

  onKnobBaseColorChanged: knobCanvas.requestPaint()

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

  WarpDragArea {
    id: mouseArea

    anchors.fill: parent
    defaultValue: root.defaultValue
    from: root.minValue
    to: root.maxValue
    value: root.value

    onDragEnded: {
      knobCanvas.requestPaint();
    }
    onEntered: {
      knobCanvas.requestPaint();
    }
    onExited: {
      if (!mouseArea.dragging) {
        knobCanvas.requestPaint();
      }
    }
    onValueModified: function (newValue) {
      root.value = newValue;
    }
  }
}
