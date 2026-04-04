// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmStyle

Arranger {
  id: root

  readonly property real maxVelocityHeight: height
  required property var pianoRoll

  function beginObjectCreation(x: real, y: real): var {
    return null;
  }

  function getVelocityAtY(y: real): int {
    return Math.round(((maxVelocityHeight - y) / maxVelocityHeight) * 127);
  }

  function getYAtVelocity(velocity: int): real {
    return maxVelocityHeight - (velocity / 127.0) * maxVelocityHeight;
  }

  function moveSelectionsY(dy: real, prevY: real) {
    const prevVelocity = getVelocityAtY(prevY);
    const currentVelocity = getVelocityAtY(prevY + dy);
    if (currentVelocity == prevVelocity) {
      return;
    }
    const velocityDelta = currentVelocity - prevVelocity;
    if (root.selectionOperator) {
      const success = root.selectionOperator.changeVelocities(velocityDelta);
      if (!success) {
        console.warn("Failed to change velocities - validation failed");
      }
    }
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
    root.tempQmlArrangerObjects.forEach(qmlObj => {
      qmlObj.height = qmlObj.heightOnConstruction - dy;
    });
  }

  editorSettings: pianoRoll.editorSettings
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: Repeater {
    id: midiNotesRepeater

    anchors.fill: parent
    model: root.clipEditor.region.midiNotes

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

          // Visual velocity bar at bottom
          VelocityBarView {
            id: velocityBar

            property bool isHovered: velocityMouseArea.containsMouse

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            arrangerObject: velocityLoader.midiNote
            height: parent.height * (velocityLoader.midiNote.velocity / 127.0)
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
                root.moveSelectionsY(dy, 0);
              }
            }
            onPressed: mouse => {
              lastY = mouse.y;
              root.handleObjectSelection(midiNotesRepeater.model, velocityLoader.index, mouse);
              root.currentAction = Arranger.ResizingUp;
            }
            onReleased: {
              root.currentAction = Arranger.None;
            }
          }
        }
      }
    }
  }
}
