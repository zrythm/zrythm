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

  model: {
    // Collect all MIDI notes
    const notes = [];
    for (let i = 0; i < root.region.midiNotes.rowCount(); i++) {
      const midiNote = root.region.midiNotes.data(root.region.midiNotes.index(i, 0), ArrangerObjectListModel.ArrangerObjectPtrRole);
      notes.push(midiNote);
    }

    // Process through loop segments
    return ArrangerObjectHelpers.processLoopedItems(root.region.loopRange, root.regionTicks, root.contentWidth, notes, function (note, absStart, absEnd) {
      const relativeStart = absStart / root.regionTicks;
      const relativeEnd = absEnd / root.regionTicks;
      const x = relativeStart * root.contentWidth;
      const width = (relativeEnd - relativeStart) * root.contentWidth;
      const relativePitch = ((note.pitch - root.minVisiblePitch) + 1);
      const y = root.contentHeight - (relativePitch * root.midiNoteHeight);

      return Qt.rect(x, y, width, root.midiNoteHeight);
    });
  }

  delegate: Rectangle {
    id: midiNoteRect

    required property rect modelData

    color: Style.regionContentColor
    height: modelData.height
    width: modelData.width
    x: modelData.x
    y: modelData.y
  }
}
