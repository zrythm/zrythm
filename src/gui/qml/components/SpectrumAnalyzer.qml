// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmGui
import ZrythmStyle

Control {
  id: root

  required property AudioEngine audioEngine
  property int fft_size: 512
  readonly property real max_frequency: sampleRate / 2
  property real min_frequency: 40.0
  property int sampleRate: audioEngine.sampleRate
  readonly property color spectrumColor: root.palette.text
  required property AudioPort stereoPort

  implicitHeight: 24
  implicitWidth: 60

  onStereoPortChanged: canvas.requestPaint()

  SpectrumAnalyzerProcessor {
    id: spectrumAnalyzer

    fftSize: root.fft_size
    stereoPort: root.stereoPort
    sampleRate: root.sampleRate

    onSpectrumDataChanged: canvas.requestPaint()
  }

  Rectangle {
    border.color: root.palette.mid
    border.width: 1
    color: root.palette.base
    radius: 2

    Canvas {
      id: canvas

      function get_bin_pos(bin, num_bins) {
        // Use C++ for frequency scaling calculations
        const scaled_freq = spectrumAnalyzer.getScaledFrequency(bin, num_bins, root.min_frequency, root.max_frequency);
        return Math.max(0, Math.min(width - 1, scaled_freq * width));
      }

      function lerp(a, b, t) {
        return a * (1 - t) + b * t;
      }

      anchors.fill: parent

      onPaint: {
        const ctx = getContext("2d");
        ctx.clearRect(0, 0, width, height);

        const data = spectrumAnalyzer.spectrumData;
        const bin_count = data.length;
        if (bin_count === 0)
          return;
        ctx.fillStyle = root.spectrumColor;

        for (let i = 0; i < bin_count - 1; i++) {
          const freq_pos = get_bin_pos(i, bin_count);
          const next_freq_pos = get_bin_pos(i + 1, bin_count);

          const power = data[i];
          const next_power = data[i + 1];

          const freq_delta = next_freq_pos - freq_pos;

          for (let j = Math.floor(freq_pos); j < Math.floor(next_freq_pos); j++) {
            const t = (j - freq_pos) / freq_delta;
            const interpolated = lerp(power, next_power, t);
            const bar_height = height * interpolated;

            ctx.fillRect(j, height - bar_height, 1, bar_height);
          }
        }

        // Draw the last bin
        const last_freq_pos = get_bin_pos(bin_count - 1, bin_count);
        const last_power = data[bin_count - 1];
        const last_bar_height = height * last_power;
        ctx.fillRect(last_freq_pos, height - last_bar_height, 1, last_bar_height);
      }
    }

    anchors {
      fill: parent
      margins: root.padding
    }
  }

  ToolTip {
    text: qsTr("Master Output Spectrum")
  }
}
