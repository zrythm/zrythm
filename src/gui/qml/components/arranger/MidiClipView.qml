// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

ClipBaseView {
  id: root

  property bool isLanePart: false
  required property TrackLane lane
  readonly property MidiClip midiClip: arrangerObject as MidiClip

  clipContent: MidiClipContent {
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    midiClip: root.midiClip
  }
}
