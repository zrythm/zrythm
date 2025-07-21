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

  function beginObjectCreation(x: real, y: real): var {
    const pitch = getPitchAtY(y);
    console.log("Midi Arranger: beginObjectCreation", x, y, pitch);

    let midiNote = objectFactory.addMidiNote(root.clipEditor.region, x / root.ruler.pxPerTick, pitch);
    root.currentAction = Arranger.CreatingResizingR;
    root.setObjectSnapshotsAtStart();
    CursorManager.setResizeEndCursor();
    root.actionObject = midiNote;
    return midiNote;
  }

  function getPitchAtY(y: real): var {
    return pianoRoll.getKeyAtY(y);
  }

  function updateCursor() {
    let cursor = "default";

    switch (root.currentAction) {
    case Arranger.None:
      switch (root.tool.toolValue) {
      case Tool.Select:
        CursorManager.setPointerCursor();
        return;
      case Tool.Edit:
        CursorManager.setPencilCursor();
        return;
      case Tool.Cut:
        CursorManager.setCutCursor();
        return;
      case Tool.Eraser:
        CursorManager.setEraserCursor();
        return;
      case Tool.Ramp:
        CursorManager.setRampCursor();
        return;
      case Tool.Audition:
        CursorManager.setAuditionCursor();
        return;
      }
      break;
    case Arranger.StartingDeleteSelection:
    case Arranger.DeleteSelecting:
    case Arranger.StartingErasing:
    case Arranger.Erasing:
      CursorManager.setEraserCursor();
      return;
    case Arranger.StartingMovingCopy:
    case Arranger.MovingCopy:
      CursorManager.setCopyCursor();
      return;
    case Arranger.StartingMovingLink:
    case Arranger.MovingLink:
      CursorManager.setLinkCursor();
      return;
    case Arranger.StartingMoving:
    case Arranger.CreatingMoving:
    case Arranger.Moving:
      CursorManager.setClosedHandCursor();
      return;
    case Arranger.StartingPanning:
    case Arranger.Panning:
      CursorManager.setClosedHandCursor();
      return;
    case Arranger.StretchingL:
      CursorManager.setStretchStartCursor();
      return;
    case Arranger.ResizingL:
      CursorManager.setResizeStartCursor();
      return;
    case Arranger.ResizingLLoop:
      CursorManager.setResizeLoopStartCursor();
      return;
    case Arranger.ResizingLFade:
      CursorManager.setFadeInCursor();
      return;
    case Arranger.StretchingR:
      CursorManager.setStretchEndCursor();
      return;
    case Arranger.CreatingResizingR:
    case Arranger.ResizingR:
      CursorManager.setResizeEndCursor();
      return;
    case Arranger.ResizingRLoop:
      CursorManager.setResizeLoopEndCursor();
      return;
    case Arranger.ResizingRFade:
    case Arranger.ResizingUpFadeOut:
      CursorManager.setFadeOutCursor();
      return;
    case Arranger.ResizingUpFadeIn:
      CursorManager.setFadeInCursor();
      return;
    case Arranger.Autofilling:
      CursorManager.setBrushCursor();
      return;
    case Arranger.StartingSelection:
    case Arranger.Selecting:
      CursorManager.setPointerCursor();
      return;
    case Arranger.Renaming:
      cursor = "text";
      break;
    case Arranger.Cutting:
      CursorManager.setCutCursor();
      return;
    case Arranger.StartingAuditioning:
    case Arranger.Auditioning:
      CursorManager.setAuditionCursor();
      return;
    }

    CursorManager.setPointerCursor();
    return;
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
      property MidiNote midiNote: arrangerObject
      readonly property real midiNoteEndX: midiNoteX + midiNoteWidth
      readonly property real midiNoteHeight: root.pianoRoll.keyHeight
      readonly property real midiNoteWidth: midiNote.bounds.length.ticks * root.ruler.pxPerTick
      readonly property real midiNoteX: midiNote.position.ticks * root.ruler.pxPerTick
      readonly property real midiNoteY: (127 - midiNote.pitch) * root.pianoRoll.keyHeight

      active: midiNoteEndX + Style.scrollLoaderBufferPx >= root.scrollX && midiNoteX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
      asynchronous: true
      height: midiNoteHeight
      visible: status === Loader.Ready
      width: midiNoteWidth

      sourceComponent: Component {
        MidiNoteView {
          arrangerObject: midiNoteLoader.midiNote
          height: midiNoteLoader.midiNoteHeight
          pxPerTick: root.ruler.pxPerTick
          track: root.clipEditor.track
          width: midiNoteLoader.midiNoteWidth
          x: midiNoteLoader.midiNoteX
          y: midiNoteLoader.midiNoteY
        }
      }
    }
  }
}
