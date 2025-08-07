// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm

Control {
  id: root

  readonly property color backgroundColor: root.palette.window
  required property AudioPort leftPort
  required property AudioPort rightPort
  readonly property color waveformColor: root.palette.text
  property real zoomLevel: 1.0

  implicitHeight: 24
  implicitWidth: 60

  onLeftPortChanged: {
    if (leftPort) {
      processor.leftPort = leftPort;
    }
  }
  onRightPortChanged: {
    if (rightPort) {
      processor.rightPort = rightPort;
    }
  }
  onWidthChanged: {
    processor.displayPoints = Math.max(64, Math.floor(root.width / 2));
  }

  WaveformViewerProcessor {
    id: processor

    bufferSize: 2048
    displayPoints: Math.max(64, Math.floor(root.width / 2))
    leftPort: root.leftPort
    rightPort: root.rightPort

    onWaveformDataChanged: canvas.requestPaint()
  }

  Canvas {
    id: canvas

    onPaint: {
      var ctx = getContext("2d");
      ctx.clearRect(0, 0, width, height);

      if (!processor.waveformData || processor.waveformData.length === 0) {
        return;
      }

      const data = processor.waveformData;
      const dataLength = data.length;
      const canvasWidth = width;
      const canvasHeight = height;
      const centerY = canvasHeight / 2;

      // Background
      ctx.fillStyle = root.backgroundColor;
      ctx.fillRect(0, 0, canvasWidth, canvasHeight);

      // Center line
      ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.2);
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(0, centerY);
      ctx.lineTo(canvasWidth, centerY);
      ctx.stroke();

      // Waveform
      ctx.strokeStyle = root.waveformColor;
      ctx.lineWidth = 1;
      ctx.beginPath();

      const barWidth = canvasWidth / dataLength;

      for (let i = 0; i < dataLength; i++) {
        const value = data[i] * root.zoomLevel;
        const x = i * barWidth;
        const barHeight = value * centerY;

        if (i === 0) {
          ctx.moveTo(x, centerY - barHeight);
        } else {
          ctx.lineTo(x, centerY - barHeight);
        }
      }

      ctx.stroke();

      // Fill under waveform
      ctx.fillStyle = Qt.rgba(root.waveformColor.r, root.waveformColor.g, root.waveformColor.b, 0.3);
      ctx.beginPath();
      ctx.moveTo(0, centerY);

      for (let i = 0; i < dataLength; i++) {
        const value = data[i] * root.zoomLevel;
        const x = i * barWidth;
        const barHeight = value * centerY;
        ctx.lineTo(x, centerY - barHeight);
      }

      ctx.lineTo(canvasWidth, centerY);
      ctx.closePath();
      ctx.fill();
    }

    anchors {
      fill: parent
      margins: root.padding
    }
  }
}
