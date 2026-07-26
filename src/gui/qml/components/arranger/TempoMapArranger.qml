// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm

Arranger {
  id: root

  required property int laneHeight
  required property int laneSpacing
  required property TempoObjectManager tempoObjectManager

  function beginObjectCreation(coordinates: point): ArrangerObject {
    // Open the macro before creation so the creation and the post-creation
    // drag-move fold into a single undo step; the arranger closes it on
    // release
    const isTimeSignature = coordinates.y > laneHeight;
    root.undoStack.beginMacro(isTimeSignature ? qsTr("Create Time Signature Object") : qsTr("Create Tempo Object"));
    const obj = createObjectAt(coordinates);
    if (obj === null) {
      root.undoStack.endMacro();
      return null;
    }

    root.creationMacroOpen = true;
    if (isTimeSignature) {
      root.selectSingleObject(tempoObjectManager.timeSignatureObjects, tempoObjectManager.timeSignatureObjects.rowCount() - 1);
    } else {
      root.selectSingleObject(tempoObjectManager.tempoObjects, tempoObjectManager.tempoObjects.rowCount() - 1);
    }
    root.currentAction = Arranger.CreatingMoving;
    CursorManager.setClosedHandCursor();
    return obj;
  }

  function createObjectAt(coordinates: point): ArrangerObject {
    const tickPosition = coordinates.x / root.ruler.pxPerTick;

    if (coordinates.y > laneHeight) {
      const musicalPosition = root.tempoMap.getMusicalPosition(tickPosition);
      const adjustedTickPosition = root.tempoMap.getTickFromMusicalPosition(musicalPosition.bar, 1, 1, 0);
      return objectCreator.addTimeSignatureObject(tempoObjectManager, 4, 4, adjustedTickPosition);
    }
    return objectCreator.addTempoObject(tempoObjectManager, 140, TempoEventWrapper.Linear, tickPosition);
  }

  function getObjectHeight(obj: ArrangerObject): real {
    return laneHeight;
  }

  function getObjectY(obj: ArrangerObject): real {
    return obj.type === ArrangerObject.TempoObject ? 0 : laneHeight + laneSpacing;
  }

  function moveSelectionsY(dy: real, prevY: real) {
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
  }

  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

  content: Item {
    id: contentItem

    anchors.fill: parent

    // Tempo automation curve drawn behind the tempo objects.
    TempoCurveCanvas {
      curveColor: palette.accent
      dragActive: root.dragState.dragMode === ArrangerDragState.DragMode.Move
      dragDeltaPx: root.dragState.dragDeltaPx
      height: root.laneHeight
      pxPerTick: root.ruler.pxPerTick
      scrollX: root.scrollX
      scrollXPlusWidth: root.scrollXPlusWidth
      selectionModel: root.arrangerSelectionModel
      tempoMap: root.tempoMap
      tempoObjectManager: root.tempoObjectManager
      width: root.scrollViewWidth
      x: root.scrollX
    }

    Repeater {
      id: tempoObjectsRepeater

      model: root.tempoObjectManager.tempoObjects

      delegate: ArrangerObjectLoader {
        id: tempoObjectLoader

        readonly property TempoObject tempoObject: arrangerObject as TempoObject

        arrangerSelectionModel: root.arrangerSelectionModel
        dragDeltaPx: root.dragState.dragDeltaPx
        dragDeltaY: root.dragState.dragDeltaY
        dragMode: root.dragState.dragMode
        height: root.laneHeight
        isLoopResize: root.dragState.isLoopResize
        model: tempoObjectsRepeater.model
        pxPerTick: root.ruler.pxPerTick
        scrollViewWidth: root.scrollViewWidth
        scrollX: root.scrollX
        unifiedObjectsModel: root.unifiedObjectsModel

        sourceComponent: Component {
          TempoObjectView {
            id: tempoObjectView

            arrangerObject: tempoObjectLoader.arrangerObject
            height: root.laneHeight
            isSelected: tempoObjectLoader.selectionTracker.isSelected
            track: null
            undoStack: root.undoStack

            onHoveredChanged: {
              root.handleObjectHover(tempoObjectView.hovered, tempoObjectView);
            }
            onSelectionRequested: function (mouse) {
              root.handleObjectSelection(tempoObjectsRepeater.model, tempoObjectLoader.index, mouse);
            }
          }
        }
      }
    }

    Repeater {
      id: timeSignatureObjectsRepeater

      model: root.tempoObjectManager.timeSignatureObjects

      delegate: ArrangerObjectLoader {
        id: timeSignatureObjectLoader

        readonly property TimeSignatureObject timeSignatureObject: arrangerObject as TimeSignatureObject

        arrangerSelectionModel: root.arrangerSelectionModel
        dragDeltaPx: root.dragState.dragDeltaPx
        dragDeltaY: root.dragState.dragDeltaY
        dragMode: root.dragState.dragMode
        height: root.laneHeight
        isLoopResize: root.dragState.isLoopResize
        model: timeSignatureObjectsRepeater.model
        pxPerTick: root.ruler.pxPerTick
        scrollViewWidth: root.scrollViewWidth
        scrollX: root.scrollX
        unifiedObjectsModel: root.unifiedObjectsModel
        y: root.laneHeight + root.laneSpacing

        sourceComponent: Component {
          TimeSignatureObjectView {
            id: timeSignatureObjectView

            arrangerObject: timeSignatureObjectLoader.arrangerObject
            height: root.laneHeight
            isSelected: timeSignatureObjectLoader.selectionTracker.isSelected
            track: null
            undoStack: root.undoStack

            onHoveredChanged: {
              root.handleObjectHover(timeSignatureObjectView.hovered, timeSignatureObjectView);
            }
            onSelectionRequested: function (mouse) {
              root.handleObjectSelection(timeSignatureObjectsRepeater.model, timeSignatureObjectLoader.index, mouse);
            }
          }
        }
      }
    }
  }
}
