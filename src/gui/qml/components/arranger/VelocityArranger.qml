// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

Arranger {
  id: root

  readonly property real maxVelocityHeight: height
  required property MidiEditor midiEditor

  function moveSelectionsY(dy: real, prevY: real) {
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
  }

  editorSettings: midiEditor
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: Repeater {
    id: midiNotesRepeater

    anchors.fill: parent
    model: (root.clipEditor.clipObject as MidiClip).midiNotes

    delegate: ArrangerObjectLoader {
      id: velocityLoader

      readonly property MidiNote midiNote: arrangerObject as MidiNote

      arrangerSelectionModel: root.arrangerSelectionModel
      height: root.maxVelocityHeight
      model: midiNotesRepeater.model
      pxPerTick: root.ruler.pxPerTick
      scrollViewWidth: root.scrollViewWidth
      scrollX: root.scrollX
      unifiedObjectsModel: root.unifiedObjectsModel
      useCustomWidth: true
      width: 6

      sourceComponent: Component {
        Item {
          id: velocityColumn

          anchors.fill: velocityLoader

          // Follow the note's left-edge X during moves and left-edge resizes
          // (the note's start moves in both).
          transform: Translate {
            x: (velocityLoader.selectionTracker.isSelected && (root.dragState.dragMode === ArrangerDragState.DragMode.Move || root.dragState.dragMode === ArrangerDragState.DragMode.ResizeFromStart)) ? root.dragState.dragDeltaPx : 0
          }

          // Visual velocity bar at bottom
          VelocityBarView {
            id: velocityBar

            property bool isHovered: velocityMouseArea.containsMouse

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            arrangerObject: velocityLoader.midiNote
            // Clamped integer velocity shown live during a velocity drag.
            // Drives both the bar height and the number label, so the preview
            // matches the per-note commit exactly (no snap on release).
            displayVelocity: {
              const base = velocityLoader.midiNote.velocity;
              if (!(velocityLoader.selectionTracker.isSelected && root.dragState.dragMode === ArrangerDragState.DragMode.Velocity))
                return base;
              const delta = Math.round(-root.dragState.dragDeltaY / root.maxVelocityHeight * 127);
              return Math.max(0, Math.min(127, base + delta));
            }
            height: parent.height * (velocityBar.displayVelocity / 127.0)
            isSelected: velocityLoader.selectionTracker.isSelected
            showVelocityText: isHovered || velocityMouseArea.pressed || root.currentAction === Arranger.ResizingUp
            track: root.clipEditor.track
            undoStack: root.undoStack
          }

          // Full-height interaction area
          MouseArea {
            id: velocityMouseArea

            property real lastY: 0

            anchors.fill: parent
            cursorShape: Qt.SizeVerCursor
            hoverEnabled: true

            onPositionChanged: mouse => {
              if (pressed) {
                const dy = mouse.y - lastY;
                lastY = mouse.y;
                root.dragState.dragDeltaY += dy;
              }
            }
            onPressed: mouse => {
              lastY = mouse.y;
              root.handleObjectSelection(midiNotesRepeater.model, velocityLoader.index, mouse);
              root.currentAction = Arranger.ResizingUp;
              root.dragState.dragDeltaY = 0;
              root.dragState.dragMode = ArrangerDragState.DragMode.Velocity;
            }
            onReleased: {
              const delta = root.maxVelocityHeight > 0 ? Math.round(-root.dragState.dragDeltaY / root.maxVelocityHeight * 127) : 0;
              if (delta !== 0)
                root.selectionOperator.changeVelocities(delta);
              root.dragState.reset();
              root.currentAction = Arranger.None;
            }
          }
        }
      }
    }
  }
}
