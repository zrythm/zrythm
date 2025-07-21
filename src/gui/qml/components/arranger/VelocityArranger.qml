// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Arranger {
  id: root

  required property var pianoRoll

  function beginObjectCreation(x: real, y: real): var {
    return null;
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
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: ListView {
    id: midiNotesListView

    anchors.fill: parent
    interactive: false
    model: region.midiNotes

    delegate: Item {
      id: midiNoteDelegate

      TextMetrics {
        id: arrangerObjectTextMetrics

        font: Style.arrangerObjectTextFont
        text: "Some text"
      }

      Loader {
        id: midiNotesLoader

      }
    }
  }
}
