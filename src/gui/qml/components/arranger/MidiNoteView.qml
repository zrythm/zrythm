// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmStyle

ArrangerObjectBaseView {
  id: root

  property ChordTrack chordTrack: null
  property int highlightMode: 0
  property MidiNote midiNote: root.arrangerObject as MidiNote
  property color noteHighlightColor: "transparent"

  function _updateHighlight() {
    if (root.highlightMode === 0 || !root.midiNote) {
      root.noteHighlightColor = "transparent";
      return;
    }

    const timelineTicks = root.midiNote.position.ticks + (root.midiNote.parentObject ? root.midiNote.parentObject.position.ticks : 0);
    const activeChord = root.chordTrack ? root.chordTrack.chordAtTicks(timelineTicks) : null;
    const activeScale = root.chordTrack ? root.chordTrack.scaleAtTicks(timelineTicks) : null;

    const noteClass = root.midiNote.pitch % 12;
    const descr = activeChord && activeChord.chordDescriptor ? activeChord.chordDescriptor : null;
    const scale = activeScale && activeScale.scale ? activeScale.scale : null;

    root.noteHighlightColor = ChordHighlighter.highlightColorForNote(root.palette.highlight, noteClass, descr, scale, root.highlightMode, 1.0);
  }

  Component.onCompleted: root._updateHighlight()
  onChordTrackChanged: root._updateHighlight()
  onHighlightModeChanged: root._updateHighlight()
  onMidiNoteChanged: root._updateHighlight()

  // React to chord track structural/positional/content changes.
  // contentChanged propagates from ChordDescriptor::changed via
  // ChordObject::propertiesChanged, so in-place descriptor edits are covered.
  Connections {
    function onContentChanged() {
      root._updateHighlight();
    }

    target: root.chordTrack?.chordRegions ?? null
  }

  Connections {
    function onContentChanged() {
      root._updateHighlight();
    }

    target: root.chordTrack?.scaleObjects ?? null
  }

  // React to note position/pitch changes (e.g. dragging).
  Connections {
    function onPropertiesChanged() {
      root._updateHighlight();
    }

    target: root.midiNote ?? null
  }

  // React to parent region position changes (e.g. moving the region).
  Connections {
    function onPropertiesChanged() {
      root._updateHighlight();
    }

    target: root.midiNote?.parentObject ?? null
  }

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: 2
  }

  Rectangle {
    anchors.fill: parent
    color: root.noteHighlightColor
    radius: 2
    visible: root.noteHighlightColor.a > 0
  }
}
