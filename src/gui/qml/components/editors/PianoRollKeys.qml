// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

// Keys on the left side of the MIDI editor
Item {
    id: root

    readonly property int startNote: 0 // Starting MIDI note (0-127)
    readonly property int endNote: 127 // Ending MIDI note
    property real keyHeight: 16 // Visual height per key in pixels
    property real keyWidth: 48 // Width of white keys
    readonly property real blackKeyWidth: keyWidth * 0.6
    required property var pianoRoll

    // Signals for interaction
    signal notePressed(int note)
    signal noteReleased(int note)
    signal noteDragged(int fromNote, int toNote)


    function isWhiteKey(note) {
        const positions = [0, 2, 4, 5, 7, 9, 11];
        return positions.includes(note % 12);
    }

    function noteName(note) {
        const names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
        const octave = Math.floor(note / 12) - 1;
        return names[note % 12] + octave;
    }

    implicitWidth: keyWidth
    implicitHeight: (endNote - startNote + 1) * keyHeight

    ColumnLayout {
        id: keysLayout

        anchors.fill: parent
        spacing: 0

        Repeater {
            id: repeater

            model: endNote - startNote + 1

            Item {
                property int midiNote: root.endNote - modelData
                property bool isPressed: false

                Layout.fillWidth: true
                Layout.preferredHeight: root.keyHeight

                // White key background
                Rectangle {
                    anchors.fill: parent
                    color: parent.isPressed ? "#88aaff" : "white"

                    border {
                        width: 1
                        color: "#cccccc"
                    }

                }

                // Black key
                Rectangle {
                    visible: !root.isWhiteKey(midiNote)
                    width: blackKeyWidth
                    height: parent.height * 0.7
                    color: parent.isPressed ? "#6699ff" : "#333333"
                    radius: 2

                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                    }

                }

                // Note label (C notes only)
                Text {
                    visible: root.isWhiteKey(midiNote) && (midiNote % 12 === 0)
                    text: root.noteName(midiNote)
                    color: parent.isPressed ? "white" : "#666666"

                    font {
                        pixelSize: 10
                        family: "Monospace"
                    }

                    anchors {
                        left: parent.left
                        leftMargin: 4
                        verticalCenter: parent.verticalCenter
                    }

                }

            }

        }

    }

    MouseArea {
        id: mouseArea

        property int currentNote: -1
        property int lastValidNote: -1

        function handleNotePress(yPos) {
            var idx = Math.floor(yPos / root.keyHeight);
            if (idx < 0 || idx >= repeater.count)
                return ;

            currentNote = root.endNote - idx;
            var keyItem = repeater.itemAt(idx);
            if (currentNote !== lastValidNote) {
                if (lastValidNote !== -1) {
                    var prevIdx = root.endNote - lastValidNote;
                    repeater.itemAt(prevIdx).isPressed = false;
                    root.noteReleased(lastValidNote);
                }
                keyItem.isPressed = true;
                root.notePressed(currentNote);
                lastValidNote = currentNote;
            }
        }

        function handleNoteRelease() {
            if (lastValidNote !== -1) {
                var prevIdx = root.endNote - lastValidNote;
                repeater.itemAt(prevIdx).isPressed = false;
                root.noteReleased(lastValidNote);
                lastValidNote = -1;
            }
            currentNote = -1;
        }

        anchors.fill: parent
        preventStealing: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: handleNotePress(mouseY)
        onPositionChanged: {
            if (pressed) {
                handleNotePress(mouseY);
            }
        }
        onReleased: handleNoteRelease()
    }

}
