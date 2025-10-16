// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

Repeater {
  id: root

  required property int contentHeight
  required property int contentWidth
  readonly property int maxVisiblePitch: (region.maxVisiblePitch - region.minVisiblePitch) < minPitchCount ? region.maxVisiblePitch + 2 : region.maxVisiblePitch
  readonly property real midiNoteHeight: contentHeight / numVisiblePitches
  readonly property int minPitchCount: 5
  readonly property int minVisiblePitch: (region.maxVisiblePitch - region.minVisiblePitch) < minPitchCount ? region.minVisiblePitch - 2 : region.minVisiblePitch
  readonly property int numVisiblePitches: (maxVisiblePitch - minVisiblePitch) + 1
  required property MidiRegion region
  readonly property real regionTicks: region.bounds.length.ticks

  model: root.region.midiNotes

  // TODO: nested repeater based on number of loops
  delegate: Rectangle {
    id: midiNoteRect

    required property var arrangerObject
    required property int index
    property MidiNote midiNote: arrangerObject
    readonly property real midiNoteEndX: x + 5

    color: Style.regionContentColor
    height: root.midiNoteHeight
    width: midiNoteEndX - x
    x: {
      const relativePosition = midiNote.position.ticks / root.regionTicks;
      return relativePosition * root.contentWidth;
    }
    y: {
      const relativePitch = ((midiNote.pitch - root.minVisiblePitch) + 1);
      return root.contentHeight - (relativePitch * root.midiNoteHeight);
    }
  }
}
