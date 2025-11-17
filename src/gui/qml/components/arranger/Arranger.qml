// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
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

  property bool altHeld
  property alias arrangerContentHeight: arrangerContent.height
  required property ItemSelectionModel arrangerSelectionModel
  required property ClipEditor clipEditor
  default property alias content: extraContent.data
  property bool ctrlHeld
  property int currentAction: Arranger.CurrentAction.None
  readonly property alias currentActionStartCoordinates: arrangerMouseArea.startCoordinates
  required property var editorSettings
  property bool enableYScroll: false
  property ArrangerObjectBaseView hoveredObject: null
  required property ArrangerObjectCreator objectCreator
  required property Ruler ruler
  property alias scrollView: scrollView
  readonly property real scrollViewHeight: scrollView.height
  readonly property real scrollViewWidth: scrollView.width
  readonly property real scrollX: root.editorSettings.x
  readonly property real scrollXPlusWidth: scrollX + scrollViewWidth
  readonly property real scrollY: root.editorSettings.y
  readonly property real scrollYPlusHeight: scrollY + scrollViewHeight
  required property ArrangerObjectSelectionOperator selectionOperator
  property bool shiftHeld
  readonly property bool shouldSnap: !root.shiftHeld && (root.snapGrid.snapToGrid || root.snapGrid.snapToEvents)
  required property SnapGrid snapGrid
  property var tempQmlArrangerObjects: []
  required property TempoMap tempoMap
  required property var tool
  required property Transport transport
  required property UndoStack undoStack
  required property UnifiedArrangerObjectsModel unifiedObjectsModel

  function calculateSnappedPosition(currentTicks: real, startTicks: real): real {
    return root.shouldSnap ? root.snapGrid.snapWithStartTicks(currentTicks, startTicks) : currentTicks;
  }

  function clearTempQmlArrangerObjects() {
    // Destroy existing temporary views before clearing the array
    for (let i = 0; i < root.tempQmlArrangerObjects.length; i++) {
      const tempView = root.tempQmlArrangerObjects[i];
      tempView.destroy();
    }

    // Clear the current array
    root.tempQmlArrangerObjects = [];
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
    const sourceIndex = root.unifiedObjectsModel.mapToSource(unifiedIndex);
    return sourceIndex.model.data(sourceIndex, ArrangerObjectListModel.ArrangerObjectPtrRole);
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
      root.arrangerSelectionModel.select(unifiedIndex, ItemSelectionModel.Toggle);
    } else if (mouse.modifiers & Qt.ShiftModifier)
      if (arrangerSelectionModel.currentIndex.valid) {
        // Range selection (TODO)
        // const range = ItemSelectionRange(arrangerSelectionModel.currentIndex, unifiedIndex);
        // arrangerSelectionModel.select(range, ItemSelectionModel.SelectCurrent);
        console.log("Range selection unimplemented");
      } else {
        arrangerSelectionModel.setCurrentIndex(unifiedIndex, ItemSelectionModel.Select);
      }
    else {
      if (!root.arrangerSelectionModel.isSelected(unifiedIndex)) {
        root.arrangerSelectionModel.clear();
      }
      root.arrangerSelectionModel.setCurrentIndex(unifiedIndex, ItemSelectionModel.Select);
    }
  }

  function selectObjectsInRectangle() {
    if (!root.ctrlHeld) {
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
    return object.arrangerObject.loopRange !== null && (object.arrangerObject.loopRange.looped || isObjectHoveredInBottomHalf);
  }

  function updateCursor() {
    switch (root.currentAction) {
    case Arranger.None:
      switch (root.tool.toolValue) {
      case Tool.Select:
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

  function updateTempQmlArrangerObjects() {
    clearTempQmlArrangerObjects();

    // Get all selected indexes from the selection model
    const selectedIndexes = root.arrangerSelectionModel.selectedIndexes;

    // Add each selected object to the array
    for (let i = 0; i < selectedIndexes.length; i++) {
      const unifiedIndex = selectedIndexes[i];
      // Map back to source model to get the actual object
      const sourceIndex = root.unifiedObjectsModel.mapToSource(unifiedIndex);
      // Get the source model
      const sourceModel = sourceIndex.model;
      if (sourceModel) {
        // Get the object from the source model using the arrangerObject role
        const object = sourceModel.data(sourceIndex, ArrangerObjectListModel.ArrangerObjectPtrRole) as ArrangerObject;
        const objectTimelineTicks = object.position.ticks + (object.parentObject ? object.parentObject.position.ticks : 0);
        const objectTimelineEndTicks = ArrangerObjectHelpers.getObjectEndTicks(object) + (object.parentObject ? object.parentObject.position.ticks : 0);
        if (object) {
          // Create a temporary view that wraps the arranger object
          const tempView = tempViewComponent.createObject(arrangerContent, {
            "arrangerObject": object,
            "x": objectTimelineTicks * root.ruler.pxPerTick,
            "width": Math.max((objectTimelineEndTicks - objectTimelineTicks) * root.ruler.pxPerTick, 20),
            "y": root.getObjectY(object),
            "coordinatesOnConstruction": Qt.point(objectTimelineTicks * root.ruler.pxPerTick, root.getObjectY(object)),
            "height": root.getObjectHeight(object),
            "z": 100
          });

          if (tempView) {
            root.tempQmlArrangerObjects.push(tempView);
          }
        }
      }
    }

    console.log("Updated temporary QML arranger objects with", root.tempQmlArrangerObjects.length, "objects");
  // console.log(tempQmlArrangerObjects);
  }

  implicitHeight: 100
  implicitWidth: 64

  onHoveredObjectChanged: {
    console.log("hovered object changed:", hoveredObject);
  }

  Connections {
    function onHoveredPointChanged() {
      root.updateCursor();
    }

    enabled: root.hoveredObject !== null
    target: root.hoveredObject
  }

  Component {
    id: tempViewComponent

    ArrangerObjectTemporaryView {
    }
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
    contentHeight: root.enableYScroll ? arrangerContent.height : scrollView.height
    contentWidth: arrangerContent.width

    Synchronizer {
      sourceObject: root.editorSettings
      sourceProperty: "x"
      targetObject: scrollView.contentItem
      targetProperty: "contentX"
    }

    Loader {
      active: root.enableYScroll
      enabled: active

      sourceComponent: Synchronizer {
        sourceObject: root.editorSettings
        sourceProperty: "y"
        targetObject: scrollView.contentItem
        targetProperty: "contentY"
      }
    }

    Item {
      id: arrangerContent

      readonly property var appWindow: ApplicationWindow.window
      property bool arrangerIsActive: activeFocus

      height: 600 // TODO: calculate height

      width: root.ruler.contentWidth

      ContextMenu.menu: Menu {
        onAboutToHide: {
          arrangerContent.arrangerIsActive = Qt.binding(function () {
            return arrangerContent.activeFocus;
          });
        }
        onAboutToShow: {
          arrangerContent.arrangerIsActive = true;
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
      }

      onActiveFocusChanged: {
        console.log("active focus", activeFocus, root);
      }
      onArrangerIsActiveChanged: {
        appWindow.activeArranger = arrangerIsActive ? root : null;
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
          }
          onPositionChanged:
          // TODO: Show drop positions, etc.
          {}
        }
      }

      // Vertical grid lines
      // FIXME: logic is copy-pasted from Ruler. find a way to have a common base
      Item {
        id: timeGrid

        // Bar lines
        Repeater {
          model: root.ruler.visibleBarCount

          delegate: Item {
            id: barItem

            readonly property int bar: root.ruler.startBar + index
            required property int index

            Rectangle {
              readonly property int barTick: root.tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0)

              color: root.palette.button
              height: arrangerContent.height
              opacity: root.ruler.barLineOpacity
              width: 1
              x: barTick * root.ruler.pxPerTick
            }

            Loader {
              active: root.ruler.pxPerBeat > root.ruler.detailMeasurePxThreshold
              visible: active

              sourceComponent: Repeater {
                model: root.tempoMap.timeSignatureNumeratorAtTick(root.tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0))

                delegate: Item {
                  id: beatItem

                  readonly property int beat: index + 1
                  readonly property int beatTick: root.tempoMap.getTickFromMusicalPosition(barItem.bar, beat, 1, 0)
                  required property int index

                  Rectangle {
                    color: root.palette.button
                    height: arrangerContent.height
                    opacity: root.ruler.beatLineOpacity
                    visible: beatItem.beat !== 1
                    width: 1
                    x: beatItem.beatTick * root.ruler.pxPerTick
                  }

                  Loader {
                    active: root.ruler.pxPerSixteenth > root.ruler.detailMeasurePxThreshold
                    visible: active

                    sourceComponent: Repeater {
                      model: 16 / root.tempoMap.timeSignatureDenominatorAtTick(root.tempoMap.getTickFromMusicalPosition(barItem.bar, beatItem.beat, 1, 0))

                      Rectangle {
                        required property int index
                        readonly property int sixteenth: index + 1
                        readonly property int sixteenthTick: root.tempoMap.getTickFromMusicalPosition(barItem.bar, beatItem.beat, sixteenth, 0)

                        color: root.palette.button
                        height: arrangerContent.height
                        opacity: root.ruler.sixteenthLineOpacity
                        visible: sixteenth !== 1
                        width: 1
                        x: sixteenthTick * root.ruler.pxPerTick
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      Item {
        id: extraContent

        anchors.fill: parent
        z: 10
      }

      // Playhead
      Rectangle {
        id: playhead

        color: Style.dangerColor
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

        border.color: Style.backgroundAppendColor
        border.width: 2
        color: Qt.alpha(Style.backgroundAppendColor, 0.1)
        height: maxY - minY
        opacity: 0.5
        visible: scrollView.currentAction === Arranger.Selecting
        width: maxX - minX
        x: minX
        y: minY
        z: 1
      }

      MouseArea {
        id: arrangerMouseArea

        property alias action: scrollView.currentAction
        property point currentCoordinates
        readonly property real currentTimelineTicks: currentCoordinates.x / root.ruler.pxPerTick
        property bool hovered: false
        property point startCoordinates
        readonly property real startTimelineTicks: startCoordinates.x / root.ruler.pxPerTick

        function calculateObjectMovementTicks() {
          const obj = root.getObjectAtCurrentIndex();
          const objTimelineTicks = ArrangerObjectHelpers.getObjectTimelinePositionInTicks(obj);
          const ticksAlreadyMoved = objTimelineTicks - startTimelineTicks;
          const ticksToMove = calculateSnappedMovementTicks(startTimelineTicks) - ticksAlreadyMoved;
          return ticksToMove;
        }

        // Returns the number of ticks that the curent selection should be moved during a drag, taking grid snapping options into account.
        // Parameter: The object (at the current selection index)'s position in ticks when the drag started
        function calculateSnappedMovementTicks(objectTicksAtDragStart: real): real {
          const unsnappedTicksSinceStart = currentTimelineTicks - startTimelineTicks;
          if (root.shouldSnap) {
            const unsnappedObjectTicks = objectTicksAtDragStart + unsnappedTicksSinceStart;
            const snappedObjectTicks = root.calculateSnappedPosition(unsnappedObjectTicks, objectTicksAtDragStart);
            return snappedObjectTicks - objectTicksAtDragStart;
          } else {
            return unsnappedTicksSinceStart;
          }
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

        function moveTemporaryObjectsX() {
          const obj = root.getObjectAtCurrentIndex();
          const xToMoveSinceStart = calculateSnappedMovementTicks(ArrangerObjectHelpers.getObjectTimelinePositionInTicks(obj)) * root.ruler.pxPerTick;
          root.tempQmlArrangerObjects.forEach(qmlObj => {
            qmlObj.x = qmlObj.coordinatesOnConstruction.x + xToMoveSinceStart;
          });
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

        onDoubleClicked: mouse => {
          console.log("doubleClicked", action);
          // create an object at the mouse position
          if (mouse.button === Qt.LeftButton) {
            let obj = root.beginObjectCreation(Qt.point(mouse.x, mouse.y));
            if (obj) {
              snapNewlyCreatedObjects();
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
          root.shiftHeld = mouse.modifiers & Qt.ShiftModifier;
          root.ctrlHeld = mouse.modifiers & Qt.ControlModifier;
          root.altHeld = mouse.modifiers & Qt.AltModifier;
          if (pressed) {
            // handle action transitions
            if (action === Arranger.StartingSelection) {
              action = Arranger.Selecting;
              if (!root.ctrlHeld) {
                root.arrangerSelectionModel.clear();
              }
            } else if (action === Arranger.StartingPanning)
              action = Arranger.Panning;
            else if (action === Arranger.StartingMoving) {
              if (root.altHeld) {
                action = Arranger.MovingLink;
              } else if (root.ctrlHeld) {
                // TODO: also check that selection does not contain unclonable objects before entering this block
                action = Arranger.MovingCopy;
              } else {
                action = Arranger.Moving;
              }

              // Update qmlObjectsBeingMoved based on current selection
              root.updateTempQmlArrangerObjects();
            } else if (action === Arranger.Moving && root.altHeld) {
              action = Arranger.MovingLink;
            } else if (action === Arranger.Moving && root.ctrlHeld) {
              action = Arranger.MovingCopy;
            } else if (action === Arranger.MovingLink && !root.altHeld) {
              action = (root.ctrlHeld) ? Arranger.MovingCopy : Arranger.Moving;
            } else if (action === Arranger.MovingCopy && !root.ctrlHeld) {
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
              moveTemporaryObjectsX();
              moveTemporaryObjectsY(dy, prevCoordinates.y);
            } else if (action == Arranger.CreatingMoving) {
              const ticksToMove = calculateObjectMovementTicks();
              moveSelectionsX(ticksToMove);
              moveSelectionsY(dy, prevCoordinates.y);
            } else if ([Arranger.ResizingL, Arranger.ResizingLLoop, Arranger.ResizingLFade].includes(action)) {
              // Apply snapping to resize endpoint
              const startTicks = root.calculateSnappedPosition(currentTimelineTicks, startTimelineTicks);

              // Calculate delta in ticks based on the resize type and direction
              let resizeType = ArrangerObjectSelectionOperator.Bounds;
              let delta = 0;
              const obj = root.getObjectAtCurrentIndex();

              // Determine resize type based on current action
              if (action === Arranger.ResizingLLoop) {
                resizeType = ArrangerObjectSelectionOperator.LoopPoints;
              } else if (action === Arranger.ResizingLFade) {
                resizeType = ArrangerObjectSelectionOperator.Fades;
              }

              // Calculate delta based on the resize type
              if ([ArrangerObjectSelectionOperator.Bounds, ArrangerObjectSelectionOperator.LoopPoints].includes(resizeType)) {
                // For bounds resize, delta is the difference in position ticks
                const originalPosTicks = ArrangerObjectHelpers.getObjectTimelinePositionInTicks(obj);
                delta = startTicks - originalPosTicks;
              } else if (resizeType === ArrangerObjectSelectionOperator.Fades) {
                // For fades resize, delta is in ticks (will be converted internally)
                if (obj.fadeRange) {
                  const originalFadeIn = obj.fadeRange.startOffset.ticks;
                  delta = startTicks - originalFadeIn;
                }
              }

              root.selectionOperator.resizeObjects(resizeType, ArrangerObjectSelectionOperator.FromStart, delta);
            } else if ([Arranger.CreatingResizingMovingR, Arranger.CreatingResizingR, Arranger.ResizingR, Arranger.ResizingRLoop, Arranger.ResizingRFade].includes(action)) {
              if (action === Arranger.CreatingResizingMovingR) {
                moveSelectionsY(dy, prevCoordinates.y);
              }
              // Apply snapping to resize endpoint
              const endTicks = root.calculateSnappedPosition(currentTimelineTicks, startTimelineTicks);
              if ([Arranger.CreatingResizingMovingR, Arranger.CreatingResizingR].includes(action)) {
                ArrangerObjectHelpers.setObjectEndFromTimelineTicks(root.getObjectAtCurrentIndex(), endTicks);
              } else {
                // Calculate delta in ticks based on the resize type and direction
                let resizeType = ArrangerObjectSelectionOperator.Bounds;
                let delta = 0;
                const obj = root.getObjectAtCurrentIndex();

                // Determine resize type based on current action
                if (action === Arranger.ResizingRLoop) {
                  resizeType = ArrangerObjectSelectionOperator.LoopPoints;
                } else if (action === Arranger.ResizingRFade) {
                  resizeType = ArrangerObjectSelectionOperator.Fades;
                }

                // Calculate delta based on the resize type
                if ([ArrangerObjectSelectionOperator.Bounds, ArrangerObjectSelectionOperator.LoopPoints].includes(resizeType)) {
                  // For bounds resize, delta is the difference in ticks
                  const originalEndTicks = ArrangerObjectHelpers.getObjectEndInTimelineTicks(obj);
                  delta = endTicks - originalEndTicks;
                } else if (resizeType === ArrangerObjectSelectionOperator.Fades) {
                  // For fades resize, delta is in ticks (will be converted internally)
                  if (obj.fadeRange) {
                    const originalFadeOut = obj.fadeRange.endOffset.ticks;
                    delta = endTicks - originalFadeOut;
                  }
                }

                root.selectionOperator.resizeObjects(resizeType, ArrangerObjectSelectionOperator.FromEnd, delta);
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
            } else if (mouse.button === Qt.LeftButton) {
              if (root.hoveredObject) {
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
                  action = Arranger.StartingMoving;
                }
              } else {
                action = Arranger.StartingSelection;
              }
            }
          }
          root.updateCursor();
        }
        onReleased: {
          if (action != Arranger.None && action != Arranger.StartingSelection) {
            if (action === Arranger.Moving || action === Arranger.MovingCopy || action === Arranger.MovingLink) {
              if (root.tempQmlArrangerObjects.length > 0) {
                const firstTempObj = root.tempQmlArrangerObjects[0];
                // Calculate the final snapped position difference
                const finalTicksDiff = (firstTempObj.x - firstTempObj.coordinatesOnConstruction.x) / root.ruler.pxPerTick;
                moveSelectionsX(finalTicksDiff);
                moveSelectionsY(firstTempObj.y - firstTempObj.coordinatesOnConstruction.y, firstTempObj.coordinatesOnConstruction.y);
              }
            } else if (action === Arranger.CreatingMoving) {
              root.undoStack.endMacro();
            }
            console.log("released after action");
          } else {
            console.log("released without action");
            if (root.hoveredObject === null) {
              root.arrangerSelectionModel.clear();
            }
          }
          action = Arranger.None;
          root.clearTempQmlArrangerObjects();
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
