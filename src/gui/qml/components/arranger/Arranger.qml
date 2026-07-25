// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm
import Qt.labs.synchronizer

Item {
  id: root

  enum CurrentAction {
    None,
    CreatingResizingR,
    CreatingResizingMovingR,
    CreatingMoving,
    ResizingL,
    ResizingLLoop,
    ResizingLFade,
    ResizingR,
    ResizingRLoop,
    ResizingRFade,
    ResizingUp,
    ResizingUpFadeIn,
    ResizingUpFadeOut,
    StretchingL,
    StretchingR,
    StartingAuditioning,
    Auditioning,
    Autofilling,
    Erasing,
    StartingErasing,
    StartingMoving,
    StartingMovingCopy,
    StartingMovingLink,
    Moving,
    MovingCopy,
    MovingLink,
    StartingChangingCurve,
    ChangingCurve,
    StartingSelection,
    Selecting,
    StartingDeleteSelection,
    DeleteSelecting,
    StartingRamp,
    Ramping,
    Cutting,
    Renaming,
    StartingPanning,
    Panning
  }

  property alias arrangerContentHeight: arrangerContent.height
  required property ItemSelectionModel arrangerSelectionModel
  // The unified model index of the object clicked with Ctrl held.
  // Saved at press time so the deferred deselect on release targets the correct object
  // even if currentIndex were to change between press and release.
  property var clickedUnifiedIndexOnPress: null
  // The clip whose children are cut when the Cut tool is used on empty
  // space in editor contexts. Null in the timeline, where cutting on empty
  // space cuts every bounded object across all tracks and lanes.
  property Clip clipContext: null
  required property ClipEditor clipEditor
  default property alias content: extraContent.data
  property int currentAction: Arranger.CurrentAction.None
  readonly property alias currentActionStartCoordinates: arrangerMouseArea.startCoordinates
  readonly property alias currentMousePosition: arrangerMouseArea.currentCoordinates
  // Shareable drag state. Defaults to a private instance; compositions that
  // need cross-arranger linkage (e.g. MidiEditorPane) pass one instance to
  // several arrangers. Read/write via root.dragState.dragMode etc.
  property ArrangerDragState dragState: ArrangerDragState {
  }
  required property EditorSettings editorSettings
  property bool enableYScroll: false
  property ArrangerObjectBaseView hoveredObject: null
  required property ArrangerObjectCreator objectCreator
  required property Ruler ruler
  property alias scrollView: scrollView
  readonly property real scrollViewHeight: scrollView.height
  readonly property real scrollViewWidth: scrollView.width
  readonly property real scrollX: root.editorSettings?.x ?? 0
  readonly property real scrollXPlusWidth: scrollX + scrollViewWidth
  readonly property real scrollY: root.editorSettings?.y ?? 0
  readonly property real scrollYPlusHeight: scrollY + scrollViewHeight
  required property ArrangerObjectSelectionOperator selectionOperator
  readonly property bool shouldSnap: !KeyboardState.shiftHeld && (root.snapGrid.snapToGrid || root.snapGrid.snapToEvents)
  required property SnapGrid snapGrid
  required property TempoMap tempoMap
  required property ArrangerTool tool
  required property Transport transport
  required property UndoStack undoStack
  required property UnifiedProxyModel unifiedObjectsModel
  // Whether the clicked object was already selected at press time (Ctrl+click only).
  // Used to defer deselection to mouse release so that Ctrl+drag always has a valid target.
  property bool wasClickedObjectSelectedOnPress: false

  // Emitted when a drop occurs on the arranger canvas. Subclasses can handle
  // this for type-specific drops (e.g. chord pad → chord editor).
  signal canvasDrop(DragEvent drop)

  function calculateSnappedTimelinePosition(currentTicks: real, startTicks: real): real {
    // Clamp: timeline positions must be at or after the timeline origin
    const clampedTicks = Math.max(0, currentTicks);
    return root.shouldSnap ? root.snapGrid.snapWithStartTicks(clampedTicks, startTicks) : clampedTicks;
  }

  function findArrangerObjectLoadersInRectRecursive(item: Item, rect: rect, recursive: bool): var {
    var hitChildren = [];
    recursive = recursive || false;

    function checkChildren(currentItem) {
      currentItem.children.forEach(child => {
        if (child && child.visible && child.width > 0 && child.height > 0) {
          if (child instanceof ArrangerObjectBaseView) {
            const childRect = child.mapToItem(arrangerMouseArea, Qt.rect(child.x, child.y, child.width, child.height));
            if (QmlUtils.rectanglesIntersect(rect, childRect)) {
              hitChildren.push(child.parent as ArrangerObjectLoader);
            }
          }

          // Recursively check children if requested
          if (recursive && child.children.length > 0) {
            checkChildren(child);
          }
        }
      });
    }

    checkChildren(item);
    return hitChildren;
  }

  function getObjectAtCurrentIndex(): ArrangerObject {
    const unifiedIndex = root.arrangerSelectionModel.currentIndex;
    if (!unifiedIndex || !unifiedIndex.valid)
      return null;
    const sourceIndex = root.unifiedObjectsModel.mapToSource(unifiedIndex);
    if (!sourceIndex || !sourceIndex.valid)
      return null;
    return sourceIndex.model.data(sourceIndex, ArrangerObjectListModel.ArrangerObjectPtrRole);
  }

  // Performs the deferred Ctrl+click toggle on mouse release.
  // Only StartingMovingCopy needs this because Ctrl+click means "toggle
  // selection" when no drag happens, whereas Alt+click and plain click do not
  // toggle — they always select.
  function handleDeferredCtrlClickToggle() {
    if (root.wasClickedObjectSelectedOnPress) {
      const idx = root.clickedUnifiedIndexOnPress;
      if (idx && idx.valid) {
        root.arrangerSelectionModel.select(idx, ItemSelectionModel.Deselect);
      }
    }
  }

  function handleObjectHover(hovered: bool, arrangerObject: ArrangerObjectBaseView) {
    if (root.hoveredObject == arrangerObject && !hovered) {
      root.hoveredObject = null;
    } else if (hovered) {
      root.hoveredObject = arrangerObject;
    }
  }

  function handleObjectSelection(sourceModel: var, index: int, mouse: var) {
    const unifiedIndex = root.unifiedObjectsModel.mapFromSource(sourceModel.index(index, 0));

    if (mouse.modifiers & Qt.ControlModifier) {
      // Defer toggle to release — on press, just ensure the object is
      // selected so that a potential Ctrl+drag always has a valid target.
      // Save whether it was previously selected so onReleased can toggle
      // it off if this turns out to be a click, not a drag.
      root.wasClickedObjectSelectedOnPress = root.arrangerSelectionModel.isSelected(unifiedIndex);
      root.clickedUnifiedIndexOnPress = unifiedIndex;
      if (!root.wasClickedObjectSelectedOnPress) {
        root.arrangerSelectionModel.setCurrentIndex(unifiedIndex, ItemSelectionModel.Select);
      } else {
        root.arrangerSelectionModel.setCurrentIndex(unifiedIndex, ItemSelectionModel.NoUpdate);
      }
    } else if (mouse.modifiers & Qt.ShiftModifier) {
      // Shift+click: add to selection (range selection not yet implemented)
      root.arrangerSelectionModel.select(unifiedIndex, ItemSelectionModel.Select);
      root.arrangerSelectionModel.setCurrentIndex(unifiedIndex, ItemSelectionModel.NoUpdate);
    } else {
      if (!root.arrangerSelectionModel.isSelected(unifiedIndex)) {
        root.arrangerSelectionModel.clear();
      }
      root.arrangerSelectionModel.setCurrentIndex(unifiedIndex, ItemSelectionModel.Select);
    }
  }

  function selectObjectsInRectangle() {
    if (!KeyboardState.ctrlHeld) {
      // Clear current selection first
      root.arrangerSelectionModel.clear();
    }

    // Use recursive search to find child objects in the selection rectangle
    const selectionRect = Qt.rect(selectionRectangle.x, selectionRectangle.y, selectionRectangle.width, selectionRectangle.height);
    const hitChildren = findArrangerObjectLoadersInRectRecursive(arrangerContent, selectionRect, true);

    // Select each found object
    hitChildren.forEach(arrangerObjectLoader => {
      const sourceModelIndex = arrangerObjectLoader.model.index(arrangerObjectLoader.index, 0);
      const unifiedModelIndex = root.unifiedObjectsModel.mapFromSource(sourceModelIndex);
      arrangerObjectLoader.arrangerSelectionModel.select(unifiedModelIndex, ItemSelectionModel.Select);
    });
  }

  function selectSingleObject(sourceModel: var, index: int) {
    const unifiedIndex = root.unifiedObjectsModel.mapFromSource(sourceModel.index(index, 0));
    root.arrangerSelectionModel.clear();
    root.arrangerSelectionModel.setCurrentIndex(unifiedIndex, ItemSelectionModel.Select);
  }

  function shouldResizeBeLoopResize(object: ArrangerObjectBaseView, fromStart: bool): bool {
    // Note: should probably check all selected objects if loopable
    const isObjectHoveredInBottomHalf = root.hoveredObject.hoveredPoint.y > ((root.hoveredObject.height * 2) / 3);
    const clipObject = object.arrangerObject as Clip;
    return clipObject !== null && (clipObject.looped || isObjectHoveredInBottomHalf);
  }

  function updateCursor() {
    updateCursorFromAction(root.currentAction);
  }

  function updateCursorFromAction(action: int) {
    switch (action) {
    case Arranger.None:
      switch (root.tool.effectiveToolValue) {
      case ArrangerTool.Select:
        if (root.hoveredObject !== null) {
          const shouldBeLoopResize = shouldResizeBeLoopResize(root.hoveredObject, root.hoveredObject.isResizeLHovered);
          if (root.hoveredObject.isResizeLHovered) {
            if (shouldBeLoopResize) {
              CursorManager.setResizeLoopStartCursor();
            } else {
              CursorManager.setResizeStartCursor();
            }
          } else if (root.hoveredObject.isResizeRHovered) {
            if (shouldBeLoopResize) {
              CursorManager.setResizeLoopEndCursor();
            } else {
              CursorManager.setResizeEndCursor();
            }
          } else {
            CursorManager.setOpenHandCursor();
          }
        } else {
          CursorManager.setPointerCursor();
        }
        return;
      case ArrangerTool.Edit:
        CursorManager.setPencilCursor();
        return;
      case ArrangerTool.Cut:
        CursorManager.setCutCursor();
        return;
      case ArrangerTool.Eraser:
        CursorManager.setEraserCursor();
        return;
      case ArrangerTool.Ramp:
        CursorManager.setRampCursor();
        return;
      case ArrangerTool.Audition:
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
    case Arranger.CreatingResizingMovingR:
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
      // Note: cursor = "text" was previously set but never used
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
  }

  implicitHeight: 100
  implicitWidth: 64

  onHoveredObjectChanged: {
    console.log("hovered object changed:", hoveredObject);
  }

  Connections {
    function onEffectiveToolValueChanged() {
      root.updateCursor();
    }

    target: root.tool
  }

  Connections {
    function onHoveredPointChanged() {
      root.updateCursor();
    }

    enabled: root.hoveredObject !== null
    target: root.hoveredObject
  }

  // Arranger background
  Rectangle {
    anchors.fill: parent
    color: "transparent"
  }

  ScrollView {
    id: scrollView

    property alias currentAction: root.currentAction

    ScrollBar.vertical.policy: root.enableYScroll ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
    anchors.fill: parent
    clip: true

    Flickable {
      id: flickable

      boundsBehavior: Flickable.StopAtBounds
      contentHeight: root.enableYScroll ? arrangerContent.height : flickable.height
      contentWidth: arrangerContent.width

      Synchronizer {
        sourceObject: root.editorSettings
        sourceProperty: "x"
        targetObject: flickable
        targetProperty: "contentX"
      }

      Loader {
        active: root.enableYScroll
        enabled: active

        sourceComponent: Synchronizer {
          sourceObject: root.editorSettings
          sourceProperty: "y"
          targetObject: flickable
          targetProperty: "contentY"
        }
      }

      Item {
        id: arrangerContent

        readonly property var appWindow: ApplicationWindow.window
        property bool arrangerIsActive: activeFocus

        height: root.enableYScroll ? 600 : flickable.height
        width: root.ruler.contentWidth

        onActiveFocusChanged: {
          console.log("active focus", activeFocus, root);
        }
        onArrangerIsActiveChanged: {
          appWindow.activeArranger = arrangerIsActive ? root : null;
        }

        Menu {
          id: arrangerContextMenu

          property bool showTimebaseMenu: false

          onAboutToHide: {
            arrangerContent.arrangerIsActive = Qt.binding(function () {
              return arrangerContent.activeFocus;
            });
          }
          onAboutToShow: {
            arrangerContent.arrangerIsActive = true;
            arrangerContextMenu.showTimebaseMenu = root.selectionOperator && root.selectionOperator.selectionHasTimebaseProviders();
          }

          MenuItem {
            text: qsTr("Copy")

            onTriggered: {}
          }

          MenuItem {
            text: qsTr("Paste")

            onTriggered: {}
          }

          MenuItem {
            action: arrangerContent.appWindow.deleteAction
          }

          MenuSeparator {
          }

          MenuItem {
            action: arrangerContent.appWindow.toggleMuteAction
          }

          MenuSeparator {
            visible: arrangerContextMenu.showTimebaseMenu
          }

          Menu {
            title: qsTr("Timebase")
            visible: arrangerContextMenu.showTimebaseMenu

            MenuItem {
              text: qsTr("Inherit from Track")

              onTriggered: root.selectionOperator.clearTimebaseOverride()
            }

            MenuItem {
              text: qsTr("Musical")

              onTriggered: root.selectionOperator.setTimebaseOverride(0)
            }

            MenuItem {
              text: qsTr("Absolute")

              onTriggered: root.selectionOperator.setTimebaseOverride(1)
            }
          }
        }

        Image {
          id: dropRectImage

          height: 25
          opacity: arrangerDropArea.containsDrag ? 0.8 : 0.0
          source: ResourceManager.getIconUrl("zrythm-dark", "zrythm.svg")
          width: 25
          x: arrangerDropArea.drag.x
          y: arrangerDropArea.drag.y
          z: 2
        }

        Rectangle {
          id: arrangerDropRect

          anchors.fill: parent
          color: "grey"
          opacity: arrangerDropArea.containsDrag ? 0.1 : 0.0

          DropArea {
            id: arrangerDropArea

            anchors.fill: parent

            onContainsDragChanged: {
              if (containsDrag) {
                const item = arrangerDropArea.drag.source as Item;
                if (!item)
                  return; // external/MIME-only drags (e.g. chord pad) have no Item source
                const size = Qt.size(item.width, item.height);
                dropRectImage.width = item.width;
                dropRectImage.height = item.height;
                item.grabToImage(function (result) {
                  dropRectImage.source = result.url;
                }, size);
              }
            }
            onDropped: drop => {
              // Handle the dropped file(s)
              console.log("Drop on arranger at coordinates", drop.x, drop.y);
              root.canvasDrop(drop);
            }
            onPositionChanged:
            // TODO: Show drop positions, etc.
            {}
          }
        }

        // Vertical grid lines
        ArrangerGridCanvas {
          barLineOpacity: root.ruler.barLineOpacity
          barShadeColor: Qt.alpha(root.palette.text, 0.04)
          beatLineOpacity: root.ruler.beatLineOpacity
          detailMeasurePxThreshold: root.ruler.detailMeasurePxThreshold
          height: arrangerContent.height
          lineColor: root.palette.button
          pxPerTick: root.ruler.pxPerTick
          scrollX: root.scrollX
          scrollXPlusWidth: root.scrollXPlusWidth
          sixteenthLineOpacity: root.ruler.sixteenthLineOpacity
          tempoMap: root.tempoMap
          width: root.scrollViewWidth
          x: root.scrollX
        }

        Item {
          id: extraContent

          anchors.fill: parent
          z: 10
        }

        // Playhead
        Rectangle {
          id: playhead

          color: ZrythmTheme.dangerColor
          height: parent.height
          width: 2
          x: root.transport.playhead.ticks * root.ruler.pxPerTick - width / 2
          z: 1000
        }

        // Selection rectangle
        Rectangle {
          id: selectionRectangle

          readonly property real maxX: Math.max(arrangerMouseArea.startCoordinates.x, arrangerMouseArea.currentCoordinates.x)
          readonly property real maxY: Math.max(arrangerMouseArea.startCoordinates.y, arrangerMouseArea.currentCoordinates.y)
          readonly property real minX: Math.min(arrangerMouseArea.startCoordinates.x, arrangerMouseArea.currentCoordinates.x)
          readonly property real minY: Math.min(arrangerMouseArea.startCoordinates.y, arrangerMouseArea.currentCoordinates.y)

          border.color: ZrythmTheme.backgroundAppendColor
          border.width: 2
          color: Qt.alpha(ZrythmTheme.backgroundAppendColor, 0.1)
          height: maxY - minY
          opacity: 0.5
          visible: scrollView.currentAction === Arranger.Selecting
          width: maxX - minX
          x: minX
          y: minY
          z: 1
        }

        // Cut line indicator for the Cut tool: spans the hovered object, or
        // the full height when over empty space (cut-all preview)
        Rectangle {
          id: cutLineIndicator

          readonly property point hoveredObjectPos: root.hoveredObject ? root.hoveredObject.mapToItem(arrangerContent, 0, 0) : Qt.point(0, 0)

          color: ZrythmTheme.dangerColor
          height: root.hoveredObject ? root.hoveredObject.height : parent.height
          visible: root.tool.effectiveToolValue === ArrangerTool.Cut && (arrangerMouseArea.hovered || root.hoveredObject !== null)
          width: 2
          x: arrangerMouseArea.currentCutTicks * root.ruler.pxPerTick - width / 2
          y: hoveredObjectPos.y
          z: 100
        }

        MouseArea {
          id: arrangerMouseArea

          property alias action: scrollView.currentAction
          property point currentCoordinates
          // Snapped (if snap is on) timeline ticks at the cursor, used for
          // the Cut tool's cut position and line indicator.
          // When hovering an object, the object view's hover position is
          // used because hover moves over objects don't reach this MouseArea.
          readonly property real currentCutTicks: {
            let px = currentCoordinates.x;
            if (root.hoveredObject !== null) {
              px = root.hoveredObject.mapToItem(arrangerMouseArea, Qt.point(root.hoveredObject.hoveredPoint.x, 0)).x;
            }
            const ticks = px / root.ruler.pxPerTick;
            return root.shouldSnap ? root.snapGrid.snapWithoutStartTicks(ticks) : ticks;
          }
          // Authoritative resize delta in ticks, updated every mouse move and
          // committed directly on release (no pixel round-trip).
          property real currentResizeDeltaTicks: 0
          readonly property real currentTimelineTicks: currentCoordinates.x / root.ruler.pxPerTick
          // Dragged object's original Y at move-drag start, used by the release
          // handler to compute the final vertical delta.
          property real dragStartObjectY: 0
          property bool hovered: false

          // Dragged object's original edge position (in ticks), captured once at
          // the start of a resize drag (constant for its duration).
          property real resizeOriginalTicks: 0
          property point startCoordinates
          readonly property real startTimelineTicks: startCoordinates.x / root.ruler.pxPerTick

          function calculateObjectMovementTicks() {
            const obj = root.getObjectAtCurrentIndex();
            const objTimelineTicks = ArrangerObjectHelper.timelineTicks(obj);
            const ticksAlreadyMoved = objTimelineTicks - startTimelineTicks;
            const ticksToMove = calculateSnappedMovementTicks(startTimelineTicks) - ticksAlreadyMoved;
            return ticksToMove;
          }

          // Returns the number of ticks that the curent selection should be moved during a drag, taking grid snapping options into account.
          // Parameter: The object (at the current selection index)'s position in ticks when the drag started
          function calculateSnappedMovementTicks(objectTicksAtDragStart: real): real {
            const unsnappedTicksSinceStart = currentTimelineTicks - startTimelineTicks;
            let ticksToMove;
            if (root.shouldSnap) {
              const unsnappedObjectTicks = objectTicksAtDragStart + unsnappedTicksSinceStart;
              const snappedObjectTicks = root.calculateSnappedTimelinePosition(unsnappedObjectTicks, objectTicksAtDragStart);
              ticksToMove = snappedObjectTicks - objectTicksAtDragStart;
            } else {
              ticksToMove = unsnappedTicksSinceStart;
            }
            return Math.max(ticksToMove, -objectTicksAtDragStart);
          }

          // Moves the selected objects by the given amount of ticks.
          function moveSelectionsX(ticksToMove: real) {
            if (root.selectionOperator) {
              const success = root.selectionOperator.moveByTicks(ticksToMove);
              if (!success) {
                console.warn("Failed to move selections - validation failed");
              }
            }
          }

          // Snaps the selected objects' positions (if snap is on).
          function snapNewlyCreatedObjects() {
            const ticksToMove = calculateObjectMovementTicks();
            moveSelectionsX(ticksToMove);
          }

          acceptedButtons: Qt.AllButtons
          anchors.fill: parent
          hoverEnabled: true
          preventStealing: true
          z: 1

          // A stolen/canceled gesture (touch recognizer, popup hide) aborts
          // without committing: reset the visual drag state and any global
          // cursor override. Arranger drags preview visually and commit only in
          // onReleased, so there is no model state to revert here.
          onCanceled: {
            action = Arranger.None;
            root.dragState.reset();
            root.wasClickedObjectSelectedOnPress = false;
            root.clickedUnifiedIndexOnPress = null;
            root.updateCursor();
            CursorManager.unsetCursor();
          }
          onDoubleClicked: mouse => {
            console.log("doubleClicked", action);
            if (mouse.button === Qt.LeftButton) {
              if (root.hoveredObject !== null) {
                action = Arranger.None;
                root.dragState.dragMode = ArrangerDragState.DragMode.None;
                CursorManager.unsetCursor();
                root.hoveredObject.objectDoubleClicked();
              } else if (root.tool.effectiveToolValue === ArrangerTool.Select || root.tool.effectiveToolValue === ArrangerTool.Edit) {
                // create an object at the mouse position
                let obj = root.beginObjectCreation(Qt.point(mouse.x, mouse.y));
                if (obj) {
                  snapNewlyCreatedObjects();
                }
              }
            }
          }
          onEntered: () => {
            hovered = true;
            root.updateCursor();
          }
          onExited: () => {
            hovered = false;
          }
          onPositionChanged: mouse => {
            const prevCoordinates = Qt.point(currentCoordinates.x, currentCoordinates.y);
            currentCoordinates = Qt.point(mouse.x, mouse.y);
            const dx = mouse.x - prevCoordinates.x;
            const dy = mouse.y - prevCoordinates.y;
            const ticksDiff = dx / root.ruler.pxPerTick;
            if (pressed) {
              // handle action transitions
              if (action === Arranger.StartingSelection) {
                action = Arranger.Selecting;
                if (!KeyboardState.ctrlHeld) {
                  root.arrangerSelectionModel.clear();
                }
              } else if (action === Arranger.StartingPanning)
                action = Arranger.Panning;
              else if ([Arranger.StartingMoving, Arranger.StartingMovingCopy, Arranger.StartingMovingLink].includes(action)) {
                if (KeyboardState.altHeld) {
                  action = Arranger.MovingLink;
                } else if (KeyboardState.ctrlHeld) {
                  // TODO: also check that selection does not contain unclonable objects before entering this block
                  action = Arranger.MovingCopy;
                } else {
                  action = Arranger.Moving;
                }

                // Activate visual drag transforms on selected delegates
                root.dragState.dragMode = ArrangerDragState.DragMode.Move;
                root.dragState.dragDeltaY = 0;
                dragStartObjectY = root.getObjectY(root.getObjectAtCurrentIndex());
              } else if (action === Arranger.Moving && KeyboardState.altHeld) {
                action = Arranger.MovingLink;
              } else if (action === Arranger.Moving && KeyboardState.ctrlHeld) {
                action = Arranger.MovingCopy;
              } else if (action === Arranger.MovingLink && !KeyboardState.altHeld) {
                action = (KeyboardState.ctrlHeld) ? Arranger.MovingCopy : Arranger.Moving;
              } else if (action === Arranger.MovingCopy && !KeyboardState.ctrlHeld) {
                action = Arranger.Moving;
              }

              // Process current action
              if (action === Arranger.Selecting) {
                // Select all objects within the selection rectangle
                root.selectObjectsInRectangle();
              } else if (action === Arranger.Panning) {
                currentCoordinates.x -= dx;
                root.editorSettings.x -= dx;
                if (root.enableYScroll) {
                  currentCoordinates.y -= dy;
                  root.editorSettings.y -= dy;
                }
              } else if ([Arranger.Moving, Arranger.MovingCopy, Arranger.MovingLink].includes(action)) {
                const obj = root.getObjectAtCurrentIndex();
                root.dragState.dragDeltaPx = calculateSnappedMovementTicks(ArrangerObjectHelper.timelineTicks(obj)) * root.ruler.pxPerTick;
                moveTemporaryObjectsY(dy, prevCoordinates.y);
              } else if (action == Arranger.CreatingMoving) {
                const ticksToMove = calculateObjectMovementTicks();
                moveSelectionsX(ticksToMove);
                moveSelectionsY(dy, prevCoordinates.y);
              } else if ([Arranger.ResizingL, Arranger.ResizingLLoop, Arranger.ResizingLFade].includes(action)) {
                // Determine resize type based on current action
                let resizeType = ArrangerObjectSelectionOperator.Bounds;
                if (action === Arranger.ResizingLLoop) {
                  resizeType = ArrangerObjectSelectionOperator.LoopPoints;
                } else if (action === Arranger.ResizingLFade) {
                  resizeType = ArrangerObjectSelectionOperator.Fades;
                }

                if (resizeType === ArrangerObjectSelectionOperator.Fades) {
                  // Fades use direct manipulation (live resizeObjects) because the
                  // plain Rectangle temp view cannot represent fade curves.
                  const startTicks = root.calculateSnappedTimelinePosition(currentTimelineTicks, startTimelineTicks);
                  const obj = root.getObjectAtCurrentIndex();
                  let delta = 0;
                  if (obj.fadeRange) {
                    delta = startTicks - obj.fadeRange.startOffset.ticks;
                  }
                  root.selectionOperator.resizeObjects(resizeType, ArrangerObjectSelectionOperator.FromStart, delta);
                } else {
                  // Bounds/LoopPoints: visual transform on real delegates
                  root.dragState.isLoopResize = (resizeType === ArrangerObjectSelectionOperator.LoopPoints);
                  if (root.dragState.dragMode !== ArrangerDragState.DragMode.ResizeFromStart) {
                    root.dragState.dragMode = ArrangerDragState.DragMode.ResizeFromStart;
                    const obj = root.getObjectAtCurrentIndex();
                    resizeOriginalTicks = ArrangerObjectHelper.timelineTicks(obj);
                  }
                  const snappedStartTicks = root.calculateSnappedTimelinePosition(currentTimelineTicks, startTimelineTicks);
                  const deltaTicks = snappedStartTicks - resizeOriginalTicks;
                  currentResizeDeltaTicks = deltaTicks;
                  root.dragState.dragDeltaPx = deltaTicks * root.ruler.pxPerTick;
                }
              } else if ([Arranger.CreatingResizingMovingR, Arranger.CreatingResizingR, Arranger.ResizingR, Arranger.ResizingRLoop, Arranger.ResizingRFade].includes(action)) {
                if (action === Arranger.CreatingResizingMovingR) {
                  moveSelectionsY(dy, prevCoordinates.y);
                }
                // Apply snapping to resize endpoint
                const endTicks = root.calculateSnappedTimelinePosition(currentTimelineTicks, startTimelineTicks);
                if ([Arranger.CreatingResizingMovingR, Arranger.CreatingResizingR].includes(action)) {
                  const obj = root.getObjectAtCurrentIndex();
                  if (endTicks > ArrangerObjectHelper.timelineTicks(obj)) {
                    ArrangerObjectHelper.setEndFromTimelineTicks(obj, endTicks);
                  }
                } else {
                  // Determine resize type based on current action
                  let resizeType = ArrangerObjectSelectionOperator.Bounds;
                  if (action === Arranger.ResizingRLoop) {
                    resizeType = ArrangerObjectSelectionOperator.LoopPoints;
                  } else if (action === Arranger.ResizingRFade) {
                    resizeType = ArrangerObjectSelectionOperator.Fades;
                  }

                  if (resizeType === ArrangerObjectSelectionOperator.Fades) {
                    // Fades use direct manipulation (live resizeObjects) because the
                    // plain Rectangle temp view cannot represent fade curves.
                    const obj = root.getObjectAtCurrentIndex();
                    let delta = 0;
                    if (obj.fadeRange) {
                      delta = endTicks - obj.fadeRange.endOffset.ticks;
                    }
                    root.selectionOperator.resizeObjects(resizeType, ArrangerObjectSelectionOperator.FromEnd, delta);
                  } else {
                    // Bounds/LoopPoints: visual transform on real delegates
                    root.dragState.isLoopResize = (resizeType === ArrangerObjectSelectionOperator.LoopPoints);
                    if (root.dragState.dragMode !== ArrangerDragState.DragMode.ResizeFromEnd) {
                      root.dragState.dragMode = ArrangerDragState.DragMode.ResizeFromEnd;
                      const obj = root.getObjectAtCurrentIndex();
                      resizeOriginalTicks = ArrangerObjectHelper.timelineEndTicks(obj);
                    }
                    const deltaTicks = endTicks - resizeOriginalTicks;
                    currentResizeDeltaTicks = deltaTicks;
                    root.dragState.dragDeltaPx = deltaTicks * root.ruler.pxPerTick;
                  }
                }
              }
            }

            root.updateCursor();
          }
          // This must push a cursor via the CursorManager
          onPressed: mouse => {
            startCoordinates = Qt.point(mouse.x, mouse.y);
            currentCoordinates = startCoordinates;
            console.log("press inside arranger", startCoordinates, "start ticks:", currentTimelineTicks);
            arrangerContent.forceActiveFocus();
            if (action === Arranger.None) {
              if (mouse.button === Qt.MiddleButton) {
                action = Arranger.StartingPanning;
              } else if (mouse.button === Qt.RightButton) {
                if (root.hoveredObject) {
                  root.hoveredObject.requestSelection(mouse);
                }
              } else if (mouse.button === Qt.LeftButton) {
                if (root.tool.effectiveToolValue === ArrangerTool.Cut) {
                  // Cut tool: cut at the (snapped) cursor position. Clicking
                  // an object selects it first (like the Select tool);
                  // clicking empty space cuts every bounded object at the
                  // cursor line.
                  if (root.hoveredObject) {
                    root.hoveredObject.requestSelection(mouse);
                    root.selectionOperator.cutObjectsAt(currentCutTicks);
                  } else {
                    root.selectionOperator.cutAllObjectsAt(currentCutTicks, root.clipContext);
                  }
                  action = Arranger.Cutting;
                } else if (root.hoveredObject) {
                  root.hoveredObject.requestSelection(mouse);
                  if (root.hoveredObject.isResizingL) {
                    if (root.shouldResizeBeLoopResize(root.hoveredObject, true)) {
                      action = Arranger.ResizingLLoop;
                    } else {
                      action = Arranger.ResizingL;
                    }
                    root.hoveredObject.isResizingL = false;
                  } else if (root.hoveredObject.isResizingR) {
                    if (root.shouldResizeBeLoopResize(root.hoveredObject, false)) {
                      action = Arranger.ResizingRLoop;
                    } else {
                      action = Arranger.ResizingR;
                    }
                    root.hoveredObject.isResizingR = false;
                  } else {
                    if (mouse.modifiers & Qt.ControlModifier) {
                      action = Arranger.StartingMovingCopy;
                    } else if (mouse.modifiers & Qt.AltModifier) {
                      action = Arranger.StartingMovingLink;
                    } else {
                      action = Arranger.StartingMoving;
                    }
                  }
                } else {
                  action = Arranger.StartingSelection;
                }
              }
            }
            root.updateCursor();
          }
          onReleased: mouse => {
            if (mouse.button === Qt.RightButton) {
              arrangerContextMenu.popup();
              return;
            }
            if (action === Arranger.StartingMovingCopy) {
              // Ctrl+click without drag — perform the deferred toggle.
              root.handleDeferredCtrlClickToggle();
            } else if ([Arranger.StartingMoving, Arranger.StartingMovingLink].includes(action)) {
              // Alt+click or plain click without drag — selection was already set
              // in handleObjectSelection, nothing more to do.
            } else if (action != Arranger.None && action != Arranger.StartingSelection) {
              if ([Arranger.Moving, Arranger.MovingCopy, Arranger.MovingLink].includes(action)) {
                const finalTicksDiff = root.dragState.dragDeltaPx / root.ruler.pxPerTick;
                const finalYDiff = root.dragState.dragDeltaY;
                const hasHorizontalMove = Math.abs(finalTicksDiff) > 0.001;
                if (action === Arranger.MovingCopy) {
                  root.undoStack.beginMacro(qsTr("Copy Objects"));
                  // This creates new object clones at the original positions, and the following move operations move the original objects
                  root.selectionOperator.cloneObjects();
                } else if (action === Arranger.MovingLink) {
                  // TODO: Link operation is not yet implemented on the C++ side
                  // (ArrangerObjectSelectionOperator has no linkObjects() method).
                  // For now this performs a plain move. Replace with a proper link
                  // operation once the C++ API is available.
                  root.undoStack.beginMacro(qsTr("Move Objects"));
                } else if (hasHorizontalMove) {
                  root.undoStack.beginMacro(qsTr("Move Objects"));
                }
                if (hasHorizontalMove || action === Arranger.MovingCopy || action === Arranger.MovingLink)
                  moveSelectionsX(finalTicksDiff);
                moveSelectionsY(finalYDiff, dragStartObjectY);
                if (hasHorizontalMove || action === Arranger.MovingCopy || action === Arranger.MovingLink)
                  root.undoStack.endMacro();
              } else if (action === Arranger.CreatingMoving) {
                root.undoStack.endMacro();
              } else if ([Arranger.ResizingL, Arranger.ResizingLLoop, Arranger.ResizingR, Arranger.ResizingRLoop].includes(action)) {
                let resizeType = ArrangerObjectSelectionOperator.Bounds;
                let direction = ArrangerObjectSelectionOperator.FromEnd;

                if ([Arranger.ResizingL, Arranger.ResizingLLoop].includes(action))
                  direction = ArrangerObjectSelectionOperator.FromStart;
                if (action === Arranger.ResizingLLoop || action === Arranger.ResizingRLoop)
                  resizeType = ArrangerObjectSelectionOperator.LoopPoints;

                if (Math.abs(currentResizeDeltaTicks) > 0.001)
                  root.selectionOperator.resizeObjects(resizeType, direction, currentResizeDeltaTicks);
                // Fades resize: already handled by direct manipulation
              }
              console.log("released after action");
            } else {
              console.log("released without action");
              if (root.hoveredObject === null) {
                root.arrangerSelectionModel.clear();
              }
            }
            action = Arranger.None;
            root.dragState.reset();
            root.wasClickedObjectSelectedOnPress = false;
            root.clickedUnifiedIndexOnPress = null;
            root.updateCursor();
          }

          StateGroup {
            id: stateGroup

            states: [
              State {
                name: "unsetCursor"
                when: !arrangerMouseArea.hovered && arrangerMouseArea.action === Arranger.CurrentAction.None && root.hoveredObject === null

                StateChangeScript {
                  script: {
                    CursorManager.unsetCursor();
                  }
                }
              }
            ]
          }
        }
      }
    }
  }
}
