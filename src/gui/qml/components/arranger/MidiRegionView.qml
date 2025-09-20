// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

RegionBaseView {
  id: root

  readonly property real contentHeight: regionContentContainer.height
  readonly property real contentWidth: regionContentContainer.width
  property bool isLanePart: false
  required property TrackLane lane
  readonly property int maxVisiblePitch: (region.maxVisiblePitch - region.minVisiblePitch) < minPitchCount ? region.maxVisiblePitch + 2 : region.maxVisiblePitch
  readonly property real midiNoteHeight: regionContentContainer.height / numVisiblePitches
  readonly property int minPitchCount: 5
  readonly property int minVisiblePitch: (region.maxVisiblePitch - region.minVisiblePitch) < minPitchCount ? region.minVisiblePitch - 2 : region.minVisiblePitch
  property int numVisiblePitches: (maxVisiblePitch - minVisiblePitch) + 1
  readonly property MidiRegion region: arrangerObject as MidiRegion

  clip: true

  regionContent: Repeater {
    id: midiNotesRepeater

    model: root.region.midiNotes

    // TODO: nested repeater based on number of loops
    delegate: Rectangle {
      id: midiNoteRect

      required property var arrangerObject
      required property int index
      property MidiNote midiNote: arrangerObject
      readonly property real midiNoteEndX: x + 5

      color: palette.text
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
}
