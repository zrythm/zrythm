// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmStyle
import ZrythmArrangement
import ZrythmGui

Arranger {
  id: root

  required property PianoRoll pianoRoll
  readonly property MidiRegion region: clipEditor.region
  readonly property Track track: clipEditor.track

  function beginObjectCreation(x: real, y: real): var {
    const pitch = getPitchAtY(y);
    console.log("Midi Arranger: beginObjectCreation", x, y, pitch);
    const tickPosition = x / root.ruler.pxPerTick;
    const localTickPosition = tickPosition - region.position.ticks;

    let midiNote = objectCreator.addMidiNote(region, localTickPosition, pitch);
    root.currentAction = Arranger.CreatingResizingMovingR;
    root.selectSingleObject(region.midiNotes, region.midiNotes.rowCount() - 1);
    CursorManager.setResizeEndCursor();

    return midiNote;
  }

  function getObjectY(obj: MidiNote): real {
    return getYAtPitch(obj.pitch);
  }

  function getPitchAtY(y: real): int {
    return pianoRoll.getKeyAtY(y);
  }

  function getYAtPitch(pitch: int): real {
    return pianoRoll.keyHeight * (127 - pitch);
  }

  function moveSelectionsY(dy: real, prevY: real) {
    const prevPitch = getPitchAtY(prevY);
    const currentPitch = getPitchAtY(prevY + dy);
    if (currentPitch == prevPitch) {
      return;
    }
    const delta = currentPitch - prevPitch;
    console.log("moving selections by", delta, "pitch");
    if (root.selectionOperator) {
      const success = root.selectionOperator.moveNotesByPitch(delta);
      if (!success) {
        console.warn("Failed to move selections - validation failed");
      }
    }
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
    const prevPitch = getPitchAtY(prevY);
    const currentPitch = getPitchAtY(prevY + dy);
    const delta = currentPitch - prevPitch;
    root.tempQmlArrangerObjects.forEach(qmlObj => {
      qmlObj.y -= delta * pianoRoll.keyHeight;
    });
  }

  editorSettings: pianoRoll.editorSettings
  enableYScroll: true
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: Repeater {
    id: midiNotesRepeater

    anchors.fill: parent
    model: root.clipEditor.region.midiNotes

    delegate: ArrangerObjectLoader {
      id: midiNoteLoader

      readonly property MidiNote midiNote: arrangerObject as MidiNote

      arrangerSelectionModel: root.arrangerSelectionModel
      height: root.pianoRoll.keyHeight
      model: root.clipEditor.region.midiNotes
      pxPerTick: root.ruler.pxPerTick
      scrollViewWidth: root.scrollViewWidth
      scrollX: root.scrollX
      unifiedObjectsModel: root.unifiedObjectsModel
      y: (127 - midiNote.pitch) * root.pianoRoll.keyHeight

      sourceComponent: Component {
        MidiNoteView {
          id: midiNoteView

          arrangerObject: midiNoteLoader.arrangerObject
          isSelected: midiNoteLoader.selectionTracker.isSelected
          track: root.clipEditor.track

          onHoveredChanged: {
            root.handleObjectHover(midiNoteView.hovered, midiNoteView);
          }
          onSelectionRequested: function (mouse) {
            root.handleObjectSelection(midiNotesRepeater.model, midiNoteLoader.index, mouse);
          }
        }
      }
    }
  }
}
