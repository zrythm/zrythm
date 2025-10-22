// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmStyle
import ZrythmArrangement
import ZrythmGui

Arranger {
  id: root

  required property AutomationEditor automationEditor
  readonly property AutomationRegion region: clipEditor.region
  readonly property Track track: clipEditor.track

  function beginObjectCreation(coordinates: point): AutomationPoint {
    const value = getNormalizedValueAtY(coordinates.y);
    console.log("Automation Arranger: beginObjectCreation", coordinates, value);
    const tickPosition = coordinates.x / root.ruler.pxPerTick;
    const localTickPosition = tickPosition - region.position.ticks;

    let automationPoint = objectCreator.addAutomationPoint(region, localTickPosition, value);
    root.currentAction = Arranger.CreatingMoving;
    root.selectSingleObject(region.automationPoints, region.automationPoints.rowCount() - 1);
    CursorManager.setResizeEndCursor();

    return automationPoint;
  }

  function getNormalizedValueAtY(y: real): real {
    return 1.0 - y / root.height;
  }

  function getObjectY(obj: AutomationPoint): real {
    return getYAtNormalizedValue(obj.value);
  }

  function getYAtNormalizedValue(normalizedValue: real): real {
    return (1.0 - normalizedValue) * root.height;
  }

  function moveSelectionsY(dy: real, prevY: real) {
    const prevValue = getNormalizedValueAtY(prevY);
    const currentValue = getNormalizedValueAtY(prevY + dy);
    if (currentValue == prevValue) {
      return;
    }
    const delta = currentValue - prevValue;
    console.log("moving selections by", delta, "value");
    if (root.selectionOperator) {
      const success = root.selectionOperator.moveAutomationPointsByDelta(delta);
      if (!success) {
        console.warn("Failed to move selections - validation failed");
      }
    }
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
    root.tempQmlArrangerObjects.forEach(qmlObj => {
      qmlObj.y += dy;
    });
  }

  editorSettings: automationEditor.editorSettings
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: Repeater {
    id: automationPointsRepeater

    anchors.fill: parent
    model: root.region.automationPoints

    delegate: ArrangerObjectLoader {
      id: automationPointLoader

      readonly property AutomationPoint automationPoint: arrangerObject as AutomationPoint

      arrangerSelectionModel: root.arrangerSelectionModel
      height: 2 * Style.buttonPadding
      model: automationPointsRepeater.model
      pxPerTick: root.ruler.pxPerTick
      scrollViewWidth: root.scrollViewWidth
      scrollX: root.scrollX
      unifiedObjectsModel: root.unifiedObjectsModel
      width: height
      y: (1.0 - automationPoint.value) * root.height

      sourceComponent: Component {
        AutomationPointView {
          id: automationPointView

          arrangerObject: automationPointLoader.arrangerObject
          isSelected: automationPointLoader.selectionTracker.isSelected
          track: root.clipEditor.track

          onHoveredChanged: {
            root.handleObjectHover(automationPointView.hovered, automationPointView);
          }
          onSelectionRequested: function (mouse) {
            root.handleObjectSelection(automationPointsRepeater.model, automationPointLoader.index, mouse);
          }
        }
      }
    }
  }
}
