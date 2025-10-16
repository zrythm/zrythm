// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui

Item {
  id: root

  required property int contentHeight
  required property int contentWidth
  required property AudioRegion region

  // Store waveform data
  property list<WaveformChannel> waveformChannels: []

  function generateWaveform() {
    if (!region || contentWidth <= 0) {
      waveformChannels = [];
      waveformCanvas.requestPaint();
      return;
    }

    // Generate waveform data
    waveformChannels = QmlUtils.getAudioRegionWaveform(region, contentWidth);
    waveformCanvas.requestPaint();
  }

  // Generate initial waveform
  Component.onCompleted: generateWaveform()
  onContentWidthChanged: generateWaveform()

  // Generate waveform when region or size changes
  onRegionChanged: generateWaveform()

  // Canvas for drawing the waveform
  Canvas {
    id: waveformCanvas

    height: root.contentHeight
    width: root.contentWidth

    onPaint: {
      const ctx = getContext("2d");
      ctx.clearRect(0, 0, width, height);

      if (root.waveformChannels.length === 0) {
        return;
      }

      const numChannels = root.waveformChannels.length;
      const channelHeight = height / numChannels;

      // Set the stroke color
      const regionColor = Style.regionContentColor;
      ctx.strokeStyle = regionColor;
      ctx.fillStyle = regionColor;
      ctx.lineWidth = 1;

      // Draw each channel in its own vertical section
      for (let ch = 0; ch < numChannels; ch++) {
        const channel = root.waveformChannels[ch];
        if (!channel)
          continue;

        const minValues = channel.minValues;
        const maxValues = channel.maxValues;
        const pixelWidth = channel.pixelWidth;

        if (minValues.length !== maxValues.length || minValues.length !== pixelWidth) {
          continue;
        }

        // Calculate the vertical bounds for this channel
        const channelTop = ch * channelHeight;
        const channelBottom = (ch + 1) * channelHeight;

        // Draw the waveform as a filled path for this channel
        ctx.beginPath();

        // Start from the first max point
        if (pixelWidth > 0) {
          const firstX = 0;
          const firstY = channelTop + channelHeight - ((maxValues[0] + 1) * channelHeight / 2);
          ctx.moveTo(firstX, firstY);
        }

        // Draw the max values (top part of waveform)
        for (let i = 1; i < pixelWidth; i++) {
          const x = i;
          const y = channelTop + channelHeight - ((maxValues[i] + 1) * channelHeight / 2);
          ctx.lineTo(x, y);
        }

        // Draw the min values (bottom part of waveform) in reverse
        for (let i = pixelWidth - 1; i >= 0; i--) {
          const x = i;
          const y = channelTop + channelHeight - ((minValues[i] + 1) * channelHeight / 2);
          ctx.lineTo(x, y);
        }

        // Close the path and fill
        ctx.closePath();
        ctx.fill();

        // Draw the center line for this channel
        ctx.strokeStyle = Qt.rgba(regionColor.r, regionColor.g, regionColor.b, 0.3);
        ctx.beginPath();
        ctx.moveTo(0, channelTop + channelHeight / 2);
        ctx.lineTo(width, channelTop + channelHeight / 2);
        ctx.stroke();

        // Reset stroke color for next iteration
        ctx.strokeStyle = regionColor;
      }

      // Draw channel separator lines (optional)
      ctx.strokeStyle = Qt.rgba(regionColor.r, regionColor.g, regionColor.b, 0.2);
      ctx.lineWidth = 1;
      for (let ch = 1; ch < numChannels; ch++) {
        const y = ch * channelHeight;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
        ctx.stroke();
      }
    }
  }
}
