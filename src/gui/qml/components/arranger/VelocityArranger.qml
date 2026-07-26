// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import Zrythm
import ZrythmStyle

Arranger {
  id: root

  readonly property real maxVelocityHeight: height
  required property MidiEditor midiEditor

  // Ramp tool: a press starts a ramp line drag (see handleCustomToolRelease)
  function handleCustomToolPress(mouse: var): bool {
    if (tool.effectiveToolValue !== ArrangerTool.Ramp) {
      return false;
    }
    currentAction = Arranger.StartingRamp;
    return true;
  }

  // Ramp tool: applies the dragged line to the selected notes' velocities
  function handleCustomToolRelease(): bool {
    if (currentAction !== Arranger.Ramping) {
      return false;
    }
    const startTicks = Math.max(0, dragStartCoordinates.x) / ruler.pxPerTick;
    const endTicks = Math.max(0, dragCurrentCoordinates.x) / ruler.pxPerTick;
    selectionOperator.rampVelocities(root.clipEditor.clipObject as MidiClip, startTicks, velocityAtY(dragStartCoordinates.y), endTicks, velocityAtY(dragCurrentCoordinates.y));
    return true;
  }

  function moveSelectionsY(dy: real, prevY: real) {
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
  }

  // Maps a y coordinate to a MIDI velocity (top of the lane = 127)
  function velocityAtY(y: real): real {
    return (1 - y / maxVelocityHeight) * 127;
  }

  editorSettings: midiEditor
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: [
    Repeater {
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

            // Full-height interaction area (disabled for the Ramp tool so
            // presses reach the arranger-wide MouseArea)
            MouseArea {
              id: velocityMouseArea

              property real lastY: 0

              anchors.fill: parent
              cursorShape: Qt.SizeVerCursor
              enabled: root.tool.effectiveToolValue !== ArrangerTool.Ramp
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
    },
    // Ramp tool drag preview: line from the press point to the cursor
    Shape {
      id: rampLineIndicator

      preferredRendererType: Shape.CurveRenderer
      visible: root.currentAction === Arranger.Ramping
      x: root.dragStartCoordinates.x
      y: root.dragStartCoordinates.y
      z: 100

      ShapePath {
        fillColor: "transparent"
        startX: 0
        startY: 0
        strokeColor: ZrythmTheme.backgroundAppendColor
        strokeWidth: 2

        PathLine {
          x: root.dragCurrentCoordinates.x - root.dragStartCoordinates.x
          y: root.dragCurrentCoordinates.y - root.dragStartCoordinates.y
        }
      }
    }
  ]
}
