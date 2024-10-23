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
    property int pixelsPerBeat: 50
    property int beatsPerBar: 4
    property int numTracks: 8
    property int trackHeight: 80
    property list<ZrythmRegion> regions
    property int totalBeats: 200 // Adjust this based on your needs

    // Function to add a new region
    function addRegion(startBeat, track) {
        // console.log(startBeat, track);
        let component = Qt.createComponent("ZrythmRegion.qml");
        // console.log(component);
        if (component.status === Component.Ready) {
            let region = component.createObject(arrangerContent, {
                "startBeat": startBeat,
                "track": track,
                "pixelsPerBeat": pixelsPerBeat,
                "trackHeight": trackHeight
            });
            // console.log(region);
            regions.push(region);
            regionsChanged();
        } else {
            console.log("error");
        }
    }

    // Function to create initial regions
    function createInitialRegions() {
        addRegion(0, 0);
        addRegion(8, 1);
        addRegion(16, 2);
        addRegion(24, 3);
    }

    width: 1000
    height: 100
    Component.onCompleted: {
        createInitialRegions();
    }

    // Arranger background
    Rectangle {
        anchors.fill: parent
        color: "#2c2c2c"
    }

    // Timeline header
    Rectangle {
        id: header

        width: parent.width
        height: 30
        color: "#3c3c3c"

        Row {
            spacing: pixelsPerBeat * beatsPerBar

            Repeater {
                model: Math.ceil(arrangerContent.width / (pixelsPerBeat * beatsPerBar))

                Label {
                    width: pixelsPerBeat * beatsPerBar
                    height: 30
                    text: index + 1
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

            }

        }

    }

    // Scrollable content area
    ScrollView {
        id: scrollView

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        contentWidth: arrangerContent.width
        contentHeight: arrangerContent.height

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

        Item {
            id: arrangerContent

            width: totalBeats * pixelsPerBeat
            height: numTracks * trackHeight

            // Grid lines
            Grid {
                columns: Math.ceil(parent.width / (pixelsPerBeat * beatsPerBar))
                rows: numTracks
                spacing: 0

                Repeater {
                    model: parent.columns * parent.rows

                    Rectangle {
                        width: pixelsPerBeat * beatsPerBar
                        height: trackHeight
                        color: "transparent"
                        border.color: "#3c3c3c"
                        border.width: 1
                    }

                }

            }

            // Regions
            Repeater {
                model: regions

                Loader {
                    sourceComponent: modelData
                    onLoaded: {
                        item.pixelsPerBeat = root.pixelsPerBeat;
                        item.trackHeight = root.trackHeight;
                    }
                }

            }

            // Add new region on double click
            MouseArea {
                anchors.fill: parent
                onDoubleClicked: {
                    let beat = Math.floor(mouseX / pixelsPerBeat);
                    let track = Math.floor(mouseY / trackHeight);
                    addRegion(beat, track);
                }
            }

        }

    }

    // Playhead
    Rectangle {
        id: playhead

        width: 2
        height: parent.height
        color: "red"
        x: 0
        z: 1000

        MouseArea {
            anchors.fill: parent
            anchors.margins: -5
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.minimumX: 0
            drag.maximumX: arrangerContent.width - playhead.width
        }

    }

    // Pan the view when dragging with middle mouse button
    MouseArea {
        property point lastPos

        anchors.fill: scrollView
        acceptedButtons: Qt.MiddleButton
        onPressed: lastPos = Qt.point(mouseX, mouseY)
        onPositionChanged: {
            if (pressed) {
                var dx = mouseX - lastPos.x;
                var dy = mouseY - lastPos.y;
                scrollView.contentItem.contentX -= dx;
                scrollView.contentItem.contentY -= dy;
                lastPos = Qt.point(mouseX, mouseY);
            }
        }
    }

}
