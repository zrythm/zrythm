// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

Item {
  id: root

  // Store automation curve data
  property var automationValues: []
  required property int contentHeight
  required property int contentWidth
  required property AutomationRegion region

  function generateAutomationCurve() {
    debounceTimer.restart();
  }

  function generateAutomationCurveImpl() {
    if (!region || contentWidth <= 0) {
      automationValues = [];
      automationCanvas.requestPaint();
      return;
    }

    // Generate automation curve data
    automationValues = QmlUtils.getAutomationRegionValues(region, contentWidth);
    automationCanvas.requestPaint();
  }

  // Generate initial automation curve
  Component.onCompleted: generateAutomationCurve()
  onContentWidthChanged: generateAutomationCurve()

  // Generate automation curve when region or size changes
  onRegionChanged: generateAutomationCurve()

  // Debouncer timer for generating automation curve
  Timer {
    id: debounceTimer

    interval: 100 // 100ms delay

    onTriggered: {
      root.generateAutomationCurveImpl();
    }
  }

  Connections {
    function onPropertiesChanged() {
      root.generateAutomationCurve();
    }

    target: root.region
  }

  Connections {
    function onContentChanged() {
      root.generateAutomationCurve();
    }

    target: root.region.automationPoints
  }

  // Canvas for drawing the automation curve
  Canvas {
    id: automationCanvas

    height: root.contentHeight
    width: root.contentWidth

    onPaint: function (region) {
      const ctx = getContext("2d");
      ctx.clearRect(0, 0, width, height);

      if (root.automationValues.length === 0) {
        return;
      }

      const values = root.automationValues;
      const pixelWidth = values.length;

      // Set the stroke color
      const regionColor = Style.regionContentColor;
      ctx.strokeStyle = regionColor;
      ctx.fillStyle = regionColor;
      ctx.lineWidth = 1;

      // Draw the automation curve
      ctx.beginPath();

      let startedPath = false;

      // Find the first valid value (not -1.0)
      let startIndex = 0;
      while (startIndex < pixelWidth && values[startIndex] < 0) {
        startIndex++;
      }

      if (startIndex < pixelWidth) {
        // Start from the first valid point
        const firstX = startIndex;
        const firstY = height - (values[startIndex] * height);
        ctx.moveTo(firstX, firstY);
        startedPath = true;

        // Draw line segments for each valid value
        for (let i = startIndex + 1; i < pixelWidth; i++) {
          if (values[i] >= 0) {
            const x = i;
            const y = height - (values[i] * height);
            ctx.lineTo(x, y);
          } else {
            // Break the path when we encounter an invalid value
            if (startedPath) {
              ctx.stroke();
              startedPath = false;
            }

            // Find the next valid value
            let nextValid = i + 1;
            while (nextValid < pixelWidth && values[nextValid] < 0) {
              nextValid++;
            }

            if (nextValid < pixelWidth) {
              // Start a new path from the next valid point
              ctx.beginPath();
              const nextX = nextValid;
              const nextY = height - (values[nextValid] * height);
              ctx.moveTo(nextX, nextY);
              startedPath = true;
              i = nextValid - 1; // Continue from the valid point
            }
          }
        }

        // Stroke the final path
        if (startedPath) {
          ctx.stroke();
        }
      }

      // Draw the center line (0.5 value)
      ctx.strokeStyle = Qt.rgba(regionColor.r, regionColor.g, regionColor.b, 0.3);
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(0, height / 2);
      ctx.lineTo(width, height / 2);
      ctx.stroke();

      // Draw automation points as small circles
      ctx.fillStyle = regionColor;
      const automationPoints = root.region.automationPoints;
      for (let i = 0; i < automationPoints.length; i++) {
        const ap = automationPoints[i];
        if (ap) {
          // Convert automation point position to pixel coordinate
          const apPos = ap.position.ticks;
          const regionPos = root.region.position.ticks;
          const regionLength = root.region.bounds.length.ticks;
          const relativePos = apPos - regionPos;
          const pixelX = (relativePos / regionLength) * width;

          if (pixelX >= 0 && pixelX < width) {
            const pixelY = height - (ap.value * height);

            // Draw a small circle for the automation point
            ctx.beginPath();
            ctx.arc(pixelX, pixelY, 3, 0, 2 * Math.PI);
            ctx.fill();
          }
        }
      }
    }
  }
}
