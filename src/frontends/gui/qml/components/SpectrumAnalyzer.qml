// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmGui

Control {
  id: root

  property alias portObservationManager: spectrumAnalyzer.portObservationManager
  property alias fftSize: spectrumAnalyzer.fftSize
  property alias sampleRate: spectrumAnalyzer.sampleRate
  property alias stereoPort: spectrumAnalyzer.stereoPort

  implicitHeight: 20
  implicitWidth: 60
  padding: 1

  background: Rectangle {
    border.color: root.palette.mid
    border.width: 1
    color: root.palette.base
    radius: 2
  }

  contentItem: SpectrumAnalyzerCanvasItem {
    id: spectrumAnalyzer

    fillColor: root.palette.base
    spectrumColor: root.palette.text
  }
}
