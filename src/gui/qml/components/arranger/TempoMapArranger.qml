// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

Arranger {
  id: root

  required property int laneHeight
  required property int laneSpacing

  function beginObjectCreation(x: real, y: real): var {
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

  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

  content: ListView {
    id: tracksListView

    anchors.fill: parent
    interactive: false
    model: listTypeModel
    spacing: root.laneSpacing

    delegate: Item {
      id: trackDelegate

      readonly property bool isTempo: type === "Tempo"
      readonly property bool isTimeSignature: !isTempo
      required property string type

      Item {
        id: mainTrackObjectsContainer

        Layout.fillWidth: true
        Layout.maximumHeight: root.laneHeight
        Layout.minimumHeight: root.laneHeight

        Repeater {
          id: scalesRepeater

          model: trackDelegate.isTempo ? root.tempoMap.tempoEvents : root.tempoMap.timeSignatureEvents

          delegate: Loader {
            id: scaleLoader

            required property var model
            readonly property real tempoEventX: model.tick * root.ruler.pxPerTick

            active: tempoEventX + 100 + Style.scrollLoaderBufferPx >= root.scrollX && tempoEventX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
            asynchronous: true
            visible: status === Loader.Ready

            sourceComponent: Component {
              Label {
                id: scaleItem

                property var tempoEvent: scaleLoader.model
                x: scaleLoader.tempoEventX
                text: tempoEvent.bpm + " (" + (tempoEvent.curve === 0 ? "constant" : "linear") + ")"
                font.bold: true
              }
            }
          }
        }
      }
    }
  }

  ListModel {
    id: listTypeModel

    ListElement {
      type: "Tempo"
    }

    ListElement {
      type: "Time Signature"
    }
  }
}
