// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

Control {
  id: root

  property alias bufferSize: viewer.bufferSize
  property alias portObservationManager: viewer.portObservationManager
  property alias stereoPort: viewer.stereoPort

  implicitHeight: 20
  implicitWidth: 60
  padding: 1

  background: Rectangle {
    border.color: root.palette.mid
    border.width: 1
    color: root.palette.base
    radius: 2
  }
  contentItem: LiveWaveformViewer {
    id: viewer

    fillColor: root.palette.base
    outlineColor: Qt.rgba(root.palette.text.r, root.palette.text.g, root.palette.text.b, 0.3)
    waveformColor: root.palette.text
  }

  ToolTip {
    text: qsTr("Master Output Visualizer")
  }
}
