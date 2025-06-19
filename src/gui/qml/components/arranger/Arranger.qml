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

    required property var editorSettings
    required property var transport
    required property var tempoMap
    required property Ruler ruler
    required property var clipEditor
    required property var tool
    required property var objectFactory
    property int currentAction: Arranger.CurrentAction.None
    readonly property alias currentActionStartCoordinates: arrangerMouseArea.startCoordinates
    property var objectSnapshotsAtStart: null // snapshots of objects at the start
    property var actionObject: null // the object that is being acted upon (reference)
    property bool enableYScroll: false
    property alias scrollView: scrollView
    default property alias content: extraContent.data
    readonly property real scrollX: root.editorSettings.x
    readonly property real scrollY: root.editorSettings.y
    readonly property real scrollViewWidth: scrollView.width
    readonly property real scrollViewHeight: scrollView.height
    readonly property real scrollXPlusWidth: scrollX + scrollViewWidth
    readonly property real scrollYPlusHeight: scrollY + scrollViewHeight

    width: ruler.width
    height: 100

    // creates snapshots of currently selected objects
    // FIXME this is pseudocode for now
    function setObjectSnapshotsAtStart() {
        if (root.objectSnapshotsAtStart) {
            root.objectSnapshotsAtStart.destroy();
        }
        // root.objectSnapshotsAtStart = selectionsClone;
    }

    function setActionObject(obj) {
        root.actionObject = obj;
    }

    // Arranger background
    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    ScrollView {
        id: scrollView

        property alias currentAction: root.currentAction

        anchors.fill: parent
        clip: true
        contentWidth: arrangerContent.width
        contentHeight: root.enableYScroll ? arrangerContent.height : scrollView.height
        ScrollBar.vertical.policy: root.enableYScroll ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff

        Binding {
            target: scrollView.contentItem
            property: "contentX"
            value: root.editorSettings.x
        }

        Connections {
            function onContentXChanged() {
                root.editorSettings.x = scrollView.contentItem.contentX;
            }

            target: scrollView.contentItem
        }

        Binding {
            target: scrollView.contentItem
            property: "contentY"
            value: root.editorSettings.y
            when: root.enableYScroll
        }

        Connections {
            function onContentYChanged() {
                root.editorSettings.y = scrollView.contentItem.contentY;
            }

            target: scrollView.contentItem
            enabled: root.enableYScroll
        }

        Item {
            id: arrangerContent

            width: root.ruler.width
            height: 600 // TODO: calculate height

            ContextMenu.menu: Menu {
                MenuItem {
                    text: qsTr("Copy")
                    onTriggered: {
                    }
                }
                MenuItem {
                    text: qsTr("Paste")
                    onTriggered: {
                    }
                }
                MenuItem {
                    text: qsTr("Delete")
                    onTriggered: {
                    }
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
                      required property int index
                      readonly property int bar: root.ruler.startBar + index
                      Rectangle {
                          readonly property int barTick: root.tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0)
                          width: 1
                          height: arrangerContent.height
                          x: barTick * root.ruler.pxPerTick
                          color: root.palette.button
                          opacity: root.ruler.barLineOpacity
                      }

                      Loader {
                          active: root.ruler.pxPerBeat > root.ruler.detailMeasurePxThreshold
                          visible: active

                          sourceComponent: Repeater {
                              model: root.tempoMap.timeSignatureAtTick(root.tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0)).numerator

                              delegate: Item {
                                  id: beatItem
                                  required property int index
                                  readonly property int beat: index + 1
                                  readonly property int beatTick: root.tempoMap.getTickFromMusicalPosition(barItem.bar, beat, 1, 0)

                                  Rectangle {
                                    width: 1
                                    height: arrangerContent.height
                                    x: beatItem.beatTick * root.ruler.pxPerTick
                                    color: root.palette.button
                                    opacity: root.ruler.beatLineOpacity
                                    visible: beatItem.beat !== 1
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
                                            width: 1
                                            height: arrangerContent.height
                                            x: sixteenthTick * root.ruler.pxPerTick
                                            color: root.palette.button
                                            opacity: root.ruler.sixteenthLineOpacity
                                            visible: sixteenth !== 1
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

                width: 2
                height: parent.height
                color: Style.dangerColor
                x: root.transport.playhead.ticks * root.ruler.pxPerTick - width / 2
                z: 1000
            }

            // Selection rectangle
            Rectangle {
                id: selectionRectangle

                readonly property real minX: Math.min(arrangerMouseArea.startCoordinates.x, arrangerMouseArea.currentCoordinates.x)
                readonly property real minY: Math.min(arrangerMouseArea.startCoordinates.y, arrangerMouseArea.currentCoordinates.y)
                readonly property real maxX: Math.max(arrangerMouseArea.startCoordinates.x, arrangerMouseArea.currentCoordinates.x)
                readonly property real maxY: Math.max(arrangerMouseArea.startCoordinates.y, arrangerMouseArea.currentCoordinates.y)

                visible: scrollView.currentAction === Arranger.Selecting
                color: Qt.alpha(Style.backgroundAppendColor, 0.1)
                border.color: Style.backgroundAppendColor
                border.width: 2
                z: 1
                opacity: 0.5
                x: minX
                y: minY
                width: maxX - minX
                height: maxY - minY
            }

            MouseArea {
                id: arrangerMouseArea

                property point startCoordinates
                property point currentCoordinates
                property alias action: scrollView.currentAction
                property bool hovered: false

                StateGroup {
                    states: State {
                        name: "unsetCursor"
                        when: !arrangerMouseArea.hovered && arrangerMouseArea.action === Arranger.CurrentAction.None
                        StateChangeScript {
                            script: CursorManager.unsetCursor()
                        }
                    }
                }

                z: 1
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                hoverEnabled: true
                preventStealing: true
                propagateComposedEvents: true
                onEntered: () => {
                    hovered = true;
                    updateCursor();
                }
                onExited: () => {
                    hovered = false;
                }
                // This must push a cursor via the CursorManager
                onPressed: (mouse) => {
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
                onDoubleClicked: (mouse) => {
                    console.log("doubleClicked", action);
                    // create an object at the mouse position
                    if (mouse.button === Qt.LeftButton) {
                        let obj = beginObjectCreation(mouse.x, mouse.y);
                    }
                }
                onPositionChanged: (mouse) => {
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
                        if (action === Arranger.Selecting) {
                        } else if (action === Arranger.Panning) {
                            currentCoordinates.x -= dx;
                            root.editorSettings.x -= dx;
                            if (root.enableYScroll) {
                                currentCoordinates.y -= dy;
                                root.editorSettings.y -= dy;
                            }
                        } else if (action === Arranger.CreatingResizingR) {
                           root.actionObject.endPosition.ticks = ticks;
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
            }

        }

    }

}
