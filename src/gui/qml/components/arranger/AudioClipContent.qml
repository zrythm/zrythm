// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui

Item {
  id: root

  required property int contentHeight
  required property int contentWidth
  required property AudioClip audioClip
  required property TempoMap tempoMap

  AudioClipWaveformCanvas {
    height: root.contentHeight
    outlineColor: Qt.lighter(ZrythmTheme.clipContentColor, 1.4)
    audioClip: root.audioClip
    tempoMap: root.tempoMap
    waveformColor: ZrythmTheme.clipContentColor
    width: root.contentWidth
  }
}
