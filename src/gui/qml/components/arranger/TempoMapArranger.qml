// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

Arranger {
  id: root

  required property int laneHeight
  required property int laneSpacing
  required property TempoObjectManager tempoObjectManager

  function beginObjectCreation(coordinates: point): ArrangerObject {
    const tickPosition = coordinates.x / root.ruler.pxPerTick;

    if (coordinates.y > laneHeight) {
      console.log("Creating time signature object");

      const musicalPosition = root.tempoMap.getMusicalPosition(tickPosition);
      const adjustedTickPosition = root.tempoMap.getTickFromMusicalPosition(musicalPosition.bar, 1, 1, 0);
      root.undoStack.beginMacro("Create Time Signature Object");
      let timeSigObj = objectCreator.addTimeSignatureObject(tempoObjectManager, 4, 4, adjustedTickPosition);
      root.currentAction = Arranger.CreatingMoving;
      root.selectSingleObject(tempoObjectManager.timeSignatureObjects, tempoObjectManager.timeSignatureObjects.rowCount() - 1);
      CursorManager.setClosedHandCursor();
      return timeSigObj;
    } else {
      console.log("Creating tempo object");
      root.undoStack.beginMacro("Create Tempo Object");
      let tempoObj = objectCreator.addTempoObject(tempoObjectManager, 140, TempoEventWrapper.Linear, tickPosition);
      root.currentAction = Arranger.CreatingMoving;
      root.selectSingleObject(tempoObjectManager.tempoObjects, tempoObjectManager.tempoObjects.rowCount() - 1);
      CursorManager.setClosedHandCursor();
      return tempoObj;
    }
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

    Repeater {
      id: tempoObjectsRepeater

      model: root.tempoObjectManager.tempoObjects

      delegate: ArrangerObjectLoader {
        id: tempoObjectLoader

        readonly property TempoObject tempoObject: arrangerObject as TempoObject

        arrangerSelectionModel: root.arrangerSelectionModel
        height: root.laneHeight
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
        height: root.laneHeight
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
