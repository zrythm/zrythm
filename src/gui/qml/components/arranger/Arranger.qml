// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
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
    required property Ruler ruler
    required property var selections
    required property var tool
    property int currentAction: Arranger.CurrentAction.None
    readonly property alias currentActionStartCoordinates: arrangerMouseArea.startCoordinates
    property var arrangerSelectionsCloneAtStart // the selections at the start of the current action (own copy)
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

    function setArrangerSelectionsCloneAtStart(selectionsClone) {
        if (root.arrangerSelectionsCloneAtStart) {
            root.arrangerSelectionsCloneAtStart.destroy();
        }
        root.arrangerSelectionsCloneAtStart = selectionsClone;
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
        contentHeight: root.enableYScroll ? arrangerContent.height : height
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

            // Vertical grid lines
            Item {
                id: timeGrid

                // Bar lines
                Repeater {
                    model: Math.ceil(arrangerContent.width / root.ruler.pxPerBar)

                    Rectangle {
                        width: 1
                        height: arrangerContent.height
                        x: index * root.ruler.pxPerBar
                        color: root.palette.button
                        opacity: root.ruler.barLineOpacity
                    }

                }

                // Beat lines
                Loader {
                    active: root.ruler.pxPerBeat > root.ruler.detailMeasurePxThreshold
                    visible: active

                    sourceComponent: Repeater {
                        model: Math.ceil(arrangerContent.width / root.ruler.pxPerBeat)

                        Rectangle {
                            width: 1
                            height: arrangerContent.height
                            x: index * ruler.pxPerBeat
                            color: root.palette.button
                            opacity: ruler.beatLineOpacity
                            visible: index % 4 !== 0
                        }

                    }

                }

                // Sixteenth lines
                Loader {
                    active: root.ruler.pxPerSixteenth > root.ruler.detailMeasurePxThreshold
                    visible: active

                    sourceComponent: Repeater {
                        model: Math.ceil(arrangerContent.width / root.ruler.pxPerSixteenth)

                        Rectangle {
                            width: 1
                            height: arrangerContent.height
                            x: index * ruler.pxPerSixteenth
                            color: root.palette.button
                            opacity: ruler.sixteenthLineOpacity
                            visible: index % 4 !== 0
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
                x: root.ruler.transport.playheadPosition.ticks * root.ruler.pxPerTick - width / 2
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
                onEntered: (mouse) => {
                    console.log("Cursor entered arranger");
                    hovered = true;
                    updateCursor();
                }
                onExited: (mouse) => {
                    console.log("Cursor exited arranger");
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
