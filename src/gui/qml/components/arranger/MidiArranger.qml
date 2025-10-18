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

    delegate: Loader {
      id: midiNoteLoader

      required property var arrangerObject
      required property int index
      property MidiNote midiNote: arrangerObject
      readonly property real midiNoteEndX: midiNoteX + midiNoteWidth
      readonly property real midiNoteHeight: root.pianoRoll.keyHeight
      readonly property real midiNoteWidth: midiNote.bounds.length.ticks * root.ruler.pxPerTick
      readonly property real midiNoteX: (midiNote.position.ticks + midiNote.parentObject.position.ticks) * root.ruler.pxPerTick
      readonly property real midiNoteY: (127 - midiNote.pitch) * root.pianoRoll.keyHeight

      active: midiNoteEndX + Style.scrollLoaderBufferPx >= root.scrollX && midiNoteX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
      asynchronous: true
      height: midiNoteHeight
      visible: status === Loader.Ready
      width: midiNoteWidth

      sourceComponent: Component {
        MidiNoteView {
          id: midiNoteView

          arrangerObject: midiNoteLoader.midiNote
          height: midiNoteLoader.midiNoteHeight
          isSelected: midiNoteSelelectionTracker.isSelected
          pxPerTick: root.ruler.pxPerTick
          track: root.clipEditor.track
          width: midiNoteLoader.midiNoteWidth
          x: midiNoteLoader.midiNoteX
          y: midiNoteLoader.midiNoteY

          onHoveredChanged: {
            root.handleObjectHover(midiNoteView.hovered, midiNoteView);
          }
          onSelectionRequested: function (mouse) {
            root.handleObjectSelection(midiNotesRepeater.model, midiNoteLoader.index, mouse);
          }
        }
      }

      SelectionTracker {
        id: midiNoteSelelectionTracker

        modelIndex: {
          root.unifiedObjectsModel.addSourceModel(midiNotesRepeater.model);
          return root.unifiedObjectsModel.mapFromSource(midiNotesRepeater.model.index(midiNoteLoader.index, 0));
        }
        selectionModel: root.arrangerSelectionModel
      }
    }
  }
}
