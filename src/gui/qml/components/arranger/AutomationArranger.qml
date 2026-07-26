// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmArrangement
import ZrythmGui
import ZrythmStyle

Arranger {
  id: root

  readonly property AutomationClip automationClip: clipEditor.clipObject as AutomationClip
  required property AutomationEditor automationEditor

  // Pixel span of the clip content (where curves live).
  readonly property real clipWidth: root.automationClip && root.automationClip.length ? root.automationClip.length.ticks * root.ruler.pxPerTick : 0
  readonly property real clipX: root.automationClip ? root.automationClip.position.ticks * root.ruler.pxPerTick : 0
  property real curveEditStartCurviness: 0
  property real curveEditStartY: 0

  // --- Curve (curviness) drag state ---
  property AutomationPoint curveEditTarget: null
  readonly property real curveTolerance: 8
  readonly property real curvinessSensitivity: 0.008
  // Canvas-local cursor X while over a draggable curve segment, else -1. The
  // renderer highlights the single emitted segment under this X.
  property real hoveredCursorX: -1
  // The automation point whose curve segment is currently hovered (drives the
  // SizeVer cursor). Set only when segmentHitTest succeeds, so it stays null
  // over looped copies (which are not draggable).
  property AutomationPoint hoveredCurvePoint: null
  readonly property Track track: clipEditor.track
  required property UuidPropertyOperator uuidPropertyOperator

  function beginObjectCreation(coordinates: point): AutomationPoint {
    const automationPoint = createObjectAt(coordinates);
    root.currentAction = Arranger.CreatingMoving;
    root.selectSingleObject(root.automationClip.automationPoints, root.automationClip.automationPoints.rowCount() - 1);
    CursorManager.setResizeEndCursor();

    return automationPoint;
  }

  function createObjectAt(coordinates: point): AutomationPoint {
    const value = getNormalizedValueAtY(coordinates.y);
    const tickPosition = coordinates.x / root.ruler.pxPerTick;
    const localTickPosition = tickPosition - root.automationClip.position.ticks;

    return objectCreator.addAutomationPoint(root.automationClip, localTickPosition, value);
  }

  function createdObjectTypeAt(coordinates: point): int {
    return ArrangerObject.AutomationPoint;
  }

  function getNormalizedValueAtY(y: real): real {
    return 1.0 - y / root.height;
  }

  function getObjectHeight(obj: AutomationPoint): real {
    return 2 * ZrythmTheme.buttonPadding; // Height of automation points
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
    if (currentValue === prevValue) {
      return;
    }
    const delta = currentValue - prevValue;
    console.debug("moving selections by", delta, "value");
    if (root.selectionOperator) {
      const success = root.selectionOperator.moveAutomationPointsByDelta(delta);
      if (!success) {
        console.warn("Failed to move selections - validation failed");
      }
    }
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
    root.dragState.dragDeltaY += dy;
  }

  editorSettings: automationEditor
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: Item {
    id: arrangerContentItem

    anchors.fill: parent

    // Curve drawn behind the points.
    AutomationClipCanvas {
      id: curveCanvas

      applyLoops: false
      automationClip: root.automationClip
      curveColor: root.track.color
      dragActive: root.dragState.dragMode === ArrangerDragState.DragMode.Move
      dragDeltaPx: root.dragState.dragDeltaPx
      dragDeltaY: root.dragState.dragDeltaY
      height: parent.height
      hoveredX: root.hoveredCursorX
      referenceWidth: root.clipWidth
      referenceX: -curveCanvas.contentLeftMargin
      selectionModel: root.arrangerSelectionModel
      width: curveCanvas.contentWidth
      x: root.clipX - curveCanvas.contentLeftMargin
      y: 0
      z: 0
    }

    Repeater {
      id: automationPointsRepeater

      anchors.fill: parent
      model: root.automationClip.automationPoints

      delegate: ArrangerObjectLoader {
        id: automationPointLoader

        readonly property AutomationPoint automationPoint: arrangerObject as AutomationPoint

        arrangerSelectionModel: root.arrangerSelectionModel
        centeredOnPosition: true
        dragDeltaPx: root.dragState.dragDeltaPx
        dragDeltaY: root.dragState.dragDeltaY
        dragMode: root.dragState.dragMode
        height: 2 * ZrythmTheme.buttonPadding
        isLoopResize: root.dragState.isLoopResize
        model: automationPointsRepeater.model
        pxPerTick: root.ruler.pxPerTick
        scrollViewWidth: root.scrollViewWidth
        scrollX: root.scrollX
        unifiedObjectsModel: root.unifiedObjectsModel
        width: height
        y: (1.0 - automationPoint.value) * parent.height - height / 2
        z: 2

        sourceComponent: Component {
          AutomationPointView {
            id: automationPointView

            arrangerObject: automationPointLoader.arrangerObject
            isSelected: automationPointLoader.selectionTracker.isSelected
            track: root.clipEditor.track
            undoStack: root.undoStack

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

    // Curviness edit handle: grab the curve line and drag up/down.
    // Sits below the point delegates (z:1 vs z:2) so hovering a point still
    // reaches the point's own MouseArea, while the curve between points reaches
    // this one. It is sized to the clip span so hover only applies there.
    MouseArea {
      id: curvinessDrag

      acceptedButtons: Qt.LeftButton
      // SizeVer when on the curve / dragging it; ArrowCursor is a fallback
      // that is normally masked — off the curve we re-establish the global
      // override via updateCursor() (see handlers), so the tool cursor shows.
      cursorShape: (root.curveEditTarget !== null || root.hoveredCurvePoint !== null) ? Qt.SizeVerCursor : Qt.ArrowCursor
      height: parent.height
      hoverEnabled: true
      preventStealing: true
      visible: root.clipWidth > 0
      width: curveCanvas.width
      x: curveCanvas.x
      y: 0
      z: 1

      onCanceled: {
        if (root.curveEditTarget !== null) {
          root.curveEditTarget.curveOpts.curviness = root.curveEditStartCurviness;
        }
        root.curveEditTarget = null;
        root.hoveredCursorX = -1;
        root.hoveredCurvePoint = null;
        root.currentAction = Arranger.None;
        root.updateCursor();
      }
      onEntered: {
        if (curvinessDrag.pressed)
          return;
        root.updateCursor();
      }
      onExited: {
        root.hoveredCurvePoint = null;
        if (!curvinessDrag.pressed) {
          if (root.curveEditTarget === null)
            root.hoveredCursorX = -1;
          root.updateCursor();
        }
      }
      onPositionChanged: mouse => {
        if (pressed) {
          if (root.curveEditTarget === null)
            return;
          mouse.accepted = true;
          const dy = root.curveEditStartY - mouse.y;
          root.curveEditTarget.curveOpts.curviness = Math.max(-1.0, Math.min(1.0, root.curveEditStartCurviness + dy * root.curvinessSensitivity));
          return;
        }
        // Hover: hit-test the curve under the cursor (curvinessDrag is
        // positioned at clipX, so mouse.x is already canvas-local).
        const target = curveCanvas.segmentHitTest(mouse.x, mouse.y, root.curveTolerance);
        root.hoveredCurvePoint = target;
        // Only record the cursor X on a draggable segment; this keeps looped
        // copies (where the hit-test fails) un-highlighted. Don't clobber the
        // highlight mid-drag (the pressed branch keeps curveEditTarget set and
        // the segment under the press must stay highlighted).
        if (root.curveEditTarget === null)
          root.hoveredCursorX = target !== null ? mouse.x : -1;
        if (target !== null) {
          // Clear the global override so this item's cursorShape (SizeVer) is
          // honored.
          CursorManager.unsetCursor();
        } else {
          // Off the curve — restore the tool/selection cursor the base would
          // normally show (curvinessDrag stole hover from arrangerMouseArea).
          root.updateCursor();
        }
      }
      onPressed: mouse => {
        // Defer to point drag/selection/creation when a point is hovered.
        if (root.hoveredObject !== null) {
          mouse.accepted = false;
          return;
        }
        const target = curveCanvas.segmentHitTest(mouse.x, mouse.y, root.curveTolerance);
        if (target === null) {
          mouse.accepted = false;
          return;
        }
        mouse.accepted = true;
        CursorManager.unsetCursor();
        root.curveEditTarget = target;
        root.curveEditStartCurviness = target.curveOpts.curviness;
        root.curveEditStartY = mouse.y;
        root.hoveredCursorX = mouse.x;
        root.currentAction = Arranger.ChangingCurve;
      }
      onReleased: mouse => {
        if (root.curveEditTarget === null) {
          mouse.accepted = false;
          root.currentAction = Arranger.None;
          return;
        }
        mouse.accepted = true;
        const opts = root.curveEditTarget.curveOpts;
        if (opts !== null && opts !== undefined) {
          const finalVal = opts.curviness;
          opts.curviness = root.curveEditStartCurviness;
          root.uuidPropertyOperator.setValueOnSubObject(root.curveEditTarget, opts, "curviness", finalVal);
        }
        root.curveEditTarget = null;
        root.hoveredCursorX = -1;
        root.currentAction = Arranger.None;
        root.updateCursor();
      }
    }
  }
}
