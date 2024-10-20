import QtQuick
//import QtQuick.Controls.Basic
import ZrythmStyle 1.0

Rectangle {
    id: root

    property int startBeat: 0
    property int track: 0
    property int lengthInBeats: 4
    property int pixelsPerBeat: 50
    property int trackHeight: 80

    width: lengthInBeats * pixelsPerBeat
    height: trackHeight
    color: "#6c9"
    border.color: "#4a7"
    border.width: 1
    x: startBeat * pixelsPerBeat
    y: track * trackHeight

    Text {
        anchors.centerIn: parent
        text: "Region"
        color: "white"
    }

    MouseArea {
        anchors.fill: parent
        drag.target: parent
        drag.axis: Drag.XAndYAxis
        drag.threshold: 0
        onReleased: {
            // Snap to grid
            var newStartBeat = Math.round(parent.x / pixelsPerBeat);
            var newTrack = Math.round(parent.y / trackHeight);
            parent.startBeat = newStartBeat;
            parent.track = newTrack;
            parent.x = newStartBeat * pixelsPerBeat;
            parent.y = newTrack * trackHeight;
        }
    }

    // Resize handles
    Rectangle {
        width: 5
        height: parent.height
        color: "white"

        MouseArea {
            anchors.fill: parent
            anchors.margins: -5
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            onPositionChanged: {
                if (drag.active) {
                    var deltaBeats = Math.round(parent.x / pixelsPerBeat);
                    root.startBeat += deltaBeats;
                    root.lengthInBeats -= deltaBeats;
                    root.lengthInBeats = Math.max(1, root.lengthInBeats);
                    parent.x = 0;
                }
            }
        }

    }

    Rectangle {
        width: 5
        height: parent.height
        anchors.right: parent.right
        color: "white"

        MouseArea {
            anchors.fill: parent
            anchors.margins: -5
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            onPositionChanged: {
                if (drag.active) {
                    var deltaBeats = Math.round(parent.x / pixelsPerBeat);
                    root.lengthInBeats += deltaBeats;
                    root.lengthInBeats = Math.max(1, root.lengthInBeats);
                    parent.x = 0;
                }
            }
        }

    }

}
