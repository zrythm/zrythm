// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle 1.0
import Zrythm 1.0

Item {
  id: root

  enum CurrentAction {
    None,
    CreatingResizingR,
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

  property var actionObject: null // the object that is being acted upon (reference)
  required property var clipEditor
  default property alias content: extraContent.data
  property int currentAction: Arranger.CurrentAction.None
  readonly property alias currentActionStartCoordinates: arrangerMouseArea.startCoordinates
  required property var editorSettings
  property bool enableYScroll: false
  required property var objectFactory
  property var objectSnapshotsAtStart: null // snapshots of objects at the start
  required property Ruler ruler
  property alias scrollView: scrollView
  readonly property real scrollViewHeight: scrollView.height
  readonly property real scrollViewWidth: scrollView.width
  readonly property real scrollX: root.editorSettings.x
  readonly property real scrollXPlusWidth: scrollX + scrollViewWidth
  readonly property real scrollY: root.editorSettings.y
  readonly property real scrollYPlusHeight: scrollY + scrollViewHeight
  required property var tempoMap
  required property var tool
  required property var transport

  function setActionObject(obj) {
    root.actionObject = obj;
  }

  // creates snapshots of currently selected objects
  // FIXME this is pseudocode for now
  function setObjectSnapshotsAtStart() {
    if (root.objectSnapshotsAtStart) {
      root.objectSnapshotsAtStart.destroy();
    }
  // root.objectSnapshotsAtStart = selectionsClone;
  }

  height: 100
  width: ruler.width

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

    Binding {
      property: "contentX"
      target: scrollView.contentItem
      value: root.editorSettings.x
    }

    Connections {
      function onContentXChanged() {
        root.editorSettings.x = scrollView.contentItem.contentX;
      }

      target: scrollView.contentItem
    }

    Binding {
      property: "contentY"
      target: scrollView.contentItem
      value: root.editorSettings.y
      when: root.enableYScroll
    }

    Connections {
      function onContentYChanged() {
        root.editorSettings.y = scrollView.contentItem.contentY;
      }

      enabled: root.enableYScroll
      target: scrollView.contentItem
    }

    Item {
      id: arrangerContent

      height: 600 // TODO: calculate height

      width: root.ruler.width

      ContextMenu.menu: Menu {
        MenuItem {
          text: qsTr("Copy")

          onTriggered: {}
        }

        MenuItem {
          text: qsTr("Paste")

          onTriggered: {}
        }

        MenuItem {
          text: qsTr("Delete")

          onTriggered: {}
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
                model: root.tempoMap.timeSignatureAtTick(root.tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0)).numerator

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
                      model: 16 / root.tempoMap.timeSignatureAtTick(root.tempoMap.getTickFromMusicalPosition(barItem.bar, beatItem.beat, 1, 0)).denominator

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
        property bool hovered: false
        property point startCoordinates

        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
        propagateComposedEvents: true
        z: 1

        onDoubleClicked: mouse => {
          console.log("doubleClicked", action);
          // create an object at the mouse position
          if (mouse.button === Qt.LeftButton) {
            let obj = beginObjectCreation(mouse.x, mouse.y);
          }
        }
        onEntered: () => {
          hovered = true;
          updateCursor();
        }
        onExited: () => {
          hovered = false;
        }
        onPositionChanged: mouse => {
          const prevCoordinates = Qt.point(currentCoordinates.x, currentCoordinates.y);
          currentCoordinates = Qt.point(mouse.x, mouse.y);
          const dx = mouse.x - prevCoordinates.x;
          const dy = mouse.y - prevCoordinates.y;
          let ticks = mouse.x / root.ruler.pxPerTick;
          if (pressed) {
            if (action === Arranger.StartingSelection)
              action = Arranger.Selecting;
            else if (action === Arranger.StartingPanning)
              action = Arranger.Panning;
            // ----------------------------------------------
            if (action === Arranger.Selecting) {} else if (action === Arranger.Panning) {
              currentCoordinates.x -= dx;
              root.editorSettings.x -= dx;
              if (root.enableYScroll) {
                currentCoordinates.y -= dy;
                root.editorSettings.y -= dy;
              }
            } else if (action === Arranger.CreatingResizingR) {
              let bounds = ArrangerObjectHelpers.getObjectBounds(root.actionObject);
              if (bounds) {
                bounds.setEndPositionTicks(ticks);
              }
            }
          }

          updateCursor();
        }
        // This must push a cursor via the CursorManager
        onPressed: mouse => {
          console.log("press inside arranger");
          startCoordinates = Qt.point(mouse.x, mouse.y);
          if (action === Arranger.None) {
            if (mouse.button === Qt.MiddleButton) {
              action = Arranger.StartingPanning;
            } else if (mouse.button === Qt.LeftButton) {
              action = Arranger.StartingSelection;
            }
          }
          updateCursor();
        }
        onReleased: {
          if (action != Arranger.None) {
            console.log("released: clearing action");
            action = Arranger.None;
          } else {
            console.log("released without action");
          }
          root.actionObject = null;
          updateCursor();
        }

        StateGroup {
          states: State {
            name: "unsetCursor"
            when: !arrangerMouseArea.hovered && arrangerMouseArea.action === Arranger.CurrentAction.None

            StateChangeScript {
              script: CursorManager.unsetCursor()
            }
          }
        }
      }
    }
  }
}
