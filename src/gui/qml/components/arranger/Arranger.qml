// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmStyle 1.0

Item {
    /*
    // Zoom with mouse wheel
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        onWheel: {
            if (wheel.modifiers & Qt.ControlModifier) {
                var zoomFactor = wheel.angleDelta.y > 0 ? 1.1 : 0.9
                pixelsPerBeat *= zoomFactor
                pixelsPerBeat = Math.max(10, Math.min(200, pixelsPerBeat))
            }
        }
    }
    */

    id: root

    required property var editorSettings
    required property Ruler ruler
    property bool enableYScroll: false
    property alias scrollView: scrollView
    property int pixelsPerBeat: 50
    property int beatsPerBar: 4
    property int numTracks: 8
    property int trackHeight: 80
    property int totalBeats: 200 // Adjust this based on your needs
    default property alias content: extraContent.data
    readonly property real scrollX: root.editorSettings.x
    readonly property real scrollY: root.editorSettings.y
    readonly property real scrollViewWidth: scrollView.width
    readonly property real scrollViewHeight: scrollView.height
    readonly property real scrollXPlusWidth: scrollX + scrollViewWidth
    readonly property real scrollYPlusHeight: scrollY + scrollViewHeight

    width: ruler.width
    height: 100

    // Arranger background
    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    // Scrollable content area
    ScrollView {
        id: scrollView

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

            width: totalBeats * pixelsPerBeat
            height: numTracks * trackHeight

            // Vertical grid lines
            Item {
                id: timeGrid

                // Bar lines
                Repeater {
                    model: Math.ceil(arrangerContent.width / ruler.pxPerBar)

                    Rectangle {
                        width: 1
                        height: arrangerContent.height
                        x: index * ruler.pxPerBar
                        color: root.palette.button
                        opacity: ruler.barLineOpacity
                    }

                }

                // Beat lines
                Loader {
                    active: ruler.pxPerBeat > ruler.detailMeasurePxThreshold
                    visible: active

                    sourceComponent: Repeater {
                        model: Math.ceil(arrangerContent.width / ruler.pxPerBeat)

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
                    active: ruler.pxPerSixteenth > ruler.detailMeasurePxThreshold
                    visible: active

                    sourceComponent: Repeater {
                        model: Math.ceil(arrangerContent.width / ruler.pxPerSixteenth)

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

        }

    }

    // Pan the view when dragging with middle mouse button
    MouseArea {
        property point lastPos

        anchors.fill: scrollView
        acceptedButtons: Qt.MiddleButton
        cursorShape: pressed ? Qt.ClosedHandCursor : undefined
        onPressed: (mouse) => {
            if (mouse.button === Qt.MiddleButton)
                lastPos = Qt.point(mouseX, mouseY);
            else
                mouse.accepted = false;
        }
        onPositionChanged: {
            if (pressed) {
                var dx = mouseX - lastPos.x;
                root.editorSettings.x -= dx;
                if (root.enableYScroll) {
                    var dy = mouseY - lastPos.y;
                    root.editorSettings.y -= dy;
                }
                lastPos = Qt.point(mouseX, mouseY);
            }
        }
    }

}
