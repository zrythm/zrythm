// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

// Keys on the left side of the MIDI editor
Item {
  id: root

  readonly property color blackKeyColor: "#333333"
  readonly property color blackKeyPressedColor: Qt.tint(blackKeyColor, Qt.alpha(root.palette.highlight, 0.9)) // "#6699ff"
  readonly property int blackKeyRadius: 2
  readonly property real blackKeyWidth: keyWidth * 0.6
  readonly property color borderColor: "#cccccc"
  readonly property int endNote: 127 // Ending MIDI note
  property real keyHeight: pianoRoll.keyHeight // Visual height per key in pixels
  property real keyWidth: 48 // Width of white keys
  readonly property color labelColor: "#666666"
  readonly property color labelPressedColor: "white"
  required property var pianoRoll
  readonly property int startNote: 0 // Starting MIDI note (0-127)
  readonly property color whiteKeyPressedColor: Qt.tint("white", Qt.alpha(root.palette.highlight, 0.6)) //"#88aaff"

  signal noteDragged(int fromNote, int toNote)

  // Signals for interaction
  signal notePressed(int note)
  signal noteReleased(int note)

  function noteName(note) {
    const names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
    const octave = Math.floor(note / 12) - 1;
    return names[note % 12] + octave;
  }

  implicitHeight: (endNote - startNote + 1) * keyHeight
  implicitWidth: keyWidth

  ColumnLayout {
    id: keysLayout

    anchors.fill: parent
    spacing: 0

    Repeater {
      id: repeater

      model: root.endNote - root.startNote + 1

      Item {
        id: keyItem

        required property int index
        property bool isPressed: false
        property int midiNote: root.endNote - index

        Layout.fillWidth: true
        Layout.preferredHeight: root.keyHeight

        // Top border for white keys when next note is white
        Rectangle {
          id: topBorder

          anchors.top: parent.top
          color: root.borderColor
          height: 1
          visible: keyItem.midiNote < root.endNote && root.pianoRoll.isWhiteKey(keyItem.midiNote) && root.pianoRoll.isNextKeyWhite(keyItem.midiNote)
          width: parent.width
        }

        // White key background
        Rectangle {
          anchors.bottom: parent.bottom
          anchors.left: parent.left
          anchors.top: topBorder.visible ? topBorder.bottom : parent.top
          border.width: 0
          color: parent.isPressed && root.pianoRoll.isWhiteKey(keyItem.midiNote) ? root.whiteKeyPressedColor : "white"
          width: parent.width
        }

        // Black key
        Rectangle {
          border.width: 0
          bottomRightRadius: root.blackKeyRadius
          color: parent.isPressed ? root.blackKeyPressedColor : root.blackKeyColor
          height: parent.height
          topRightRadius: root.blackKeyRadius
          visible: root.pianoRoll.isBlackKey(keyItem.midiNote)
          width: root.blackKeyWidth

          anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
          }

          // Right-side horizontal line
          Rectangle {
            color: root.borderColor
            height: 1
            visible: parent.visible
            width: keyItem.width - root.blackKeyWidth

            anchors {
              left: parent.right
              verticalCenter: parent.verticalCenter
            }
          }
        }

        // Note label (C notes only)
        Text {
          color: parent.isPressed ? root.labelPressedColor : root.labelColor
          text: root.noteName(keyItem.midiNote)
          visible: root.pianoRoll.isWhiteKey(keyItem.midiNote) && (keyItem.midiNote % 12 === 0)

          font {
            family: "Monospace"
            pixelSize: 10
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
        return;

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

    acceptedButtons: Qt.LeftButton | Qt.RightButton
    anchors.fill: parent
    preventStealing: true

    onPositionChanged: {
      if (pressed)
        handleNotePress(mouseY);
    }
    onPressed: handleNotePress(mouseY)
    onReleased: handleNoteRelease()
  }
}
