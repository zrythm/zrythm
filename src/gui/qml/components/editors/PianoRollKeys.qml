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
    property real keyHeight: pianoRoll.keyHeight // Visual height per key in pixels
    property real keyWidth: 48 // Width of white keys
    readonly property real blackKeyWidth: keyWidth * 0.6
    required property var pianoRoll
    readonly property color borderColor: "#cccccc"
    readonly property color blackKeyPressedColor: Qt.tint(blackKeyColor, Qt.alpha(root.palette.highlight, 0.9)) // "#6699ff"
    readonly property color whiteKeyPressedColor: Qt.tint("white", Qt.alpha(root.palette.highlight, 0.6)) //"#88aaff"
    readonly property color blackKeyColor: "#333333"
    readonly property int blackKeyRadius: 2
    readonly property color labelColor: "#666666"
    readonly property color labelPressedColor: "white"
    readonly property int numOctaves: 10
    readonly property int whiteKeyCount: numOctaves * 7 + 5
    readonly property int numNotes: endNote - startNote + 1

    // Signals for interaction
    signal notePressed(int note)
    signal noteReleased(int note)
    signal noteDragged(int fromNote, int toNote)

    function noteName(note) {
        const names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
        const octave = Math.floor(note / 12) - 1;
        return names[note % 12] + octave;
    }

    implicitWidth: keyWidth
    implicitHeight: numNotes * keyHeight

    ColumnLayout {
        id: whiteKeysLayout

        anchors {
          fill: parent
          topMargin: - pianoRoll.keyHeight * 0.5
        }
        spacing: 0
        // y: - pianoRoll.keyHeight // * 0.5

        Repeater {
            id: whiteKeysRepeater

            model: root.numNotes

            Item {
                id: whiteKeyItem

                readonly property int midiNote: root.endNote - modelData
                property bool isPressed: false
                readonly property int noteHeight: root.pianoRoll.getAdjustedKeyHeight(midiNote, root.keyHeight)
                visible: root.pianoRoll.isWhiteKey(midiNote)

                Layout.fillWidth: true
                Layout.preferredHeight: noteHeight

                // White key background
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    height: whiteKeyItem.noteHeight
                    width: parent.width
                    color: parent.isPressed && root.pianoRoll.isWhiteKey(midiNote) ? root.whiteKeyPressedColor : "white"
                    border.width: 1
                    topRightRadius: root.blackKeyRadius
                    bottomRightRadius: root.blackKeyRadius
                }

                // Note label (C notes only)
                Text {
                    visible: root.pianoRoll.isWhiteKey(midiNote) && (midiNote % 12 === 0)
                    text: root.noteName(midiNote)
                    color: parent.isPressed ? root.labelPressedColor : root.labelColor

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

    ColumnLayout {
        id: blackKeysLayout

        anchors.fill: parent
        spacing: 0
        y: root.pianoRoll.getAdjustedKeyHeight(root.endNote, root.keyHeight) - pianoRoll.keyHeight * 0.5

        Repeater {
            id: blackKeysRepeater

            model: root.numNotes

            Item {
                id: blackKeyItem

                readonly property int midiNote: root.endNote - modelData
                // visible: root.pianoRoll.isBlackKey(midiNote)

                property bool isPressed: false

                // Layout.fillWidth: true
                Layout.preferredHeight: root.keyHeight
                // Black key
                Rectangle {
                    visible: root.pianoRoll.isBlackKey(midiNote)
                    width: root.blackKeyWidth
                    height: parent.height
                    color: parent.isPressed ? root.blackKeyPressedColor : root.blackKeyColor
                    topRightRadius: root.blackKeyRadius
                    bottomRightRadius: root.blackKeyRadius
                    border.width: 0

                    anchors {
                        left: parent.left
                        // verticalCenter: parent.verticalCenter
                    }
                }
                // Transparent white key
                Rectangle {
                    visible: root.pianoRoll.isWhiteKey(midiNote)
                    width: root.blackKeyWidth
                    height: root.pianoRoll.getAdjustedKeyHeight(midiNote, root.keyWidth)
                    color: "transparent"
                    border.width: 0

                    anchors {
                        left: parent.left
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
            if (idx < 0 || idx >= root.numNotes)
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
            if (pressed)
                handleNotePress(mouseY);

        }
        onReleased: handleNoteRelease()
    }

}
