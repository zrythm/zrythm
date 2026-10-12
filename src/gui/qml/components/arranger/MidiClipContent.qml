// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui

Item {
  id: root

  // Ink for the notes, derived from the clip/track color (DESIGN.md "Clips")
  property color inkColor
  required property int contentHeight
  required property int contentWidth
  property bool loopPreview: false
  required property MidiClip midiClip
  property real referenceWidth: 0
  property real referenceX: 0

  MidiClipCanvas {
    height: root.contentHeight
    loopPreview: root.loopPreview
    midiClip: root.midiClip
    noteColor: root.inkColor
    referenceWidth: root.referenceWidth
    referenceX: root.referenceX
    width: root.contentWidth
  }
}
