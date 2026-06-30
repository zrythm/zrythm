// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
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
  readonly property real keyHeight: midiEditor.keyHeight // Visual height per key in pixels
  readonly property real keyWidth: 48 // Width of white keys
  readonly property color labelColor: "#666666"
  readonly property color labelPressedColor: "white"
  required property MidiEditor midiEditor
  required property MidiActivityProvider noteActivityProvider
  readonly property int startNote: 0 // Starting MIDI note (0-127)
  readonly property color whiteKeyPressedColor: Qt.tint("white", Qt.alpha(root.palette.highlight, 0.6)) //"#88aaff"

  // Highlighting state — set by parent based on playhead position
  property ChordTrack chordTrack: null
  property ChordObject activeChord: null
  property ScaleObject activeScale: null
  property int highlightMode: 0

  property var _highlightCache: new Array(12).fill("transparent")

  function _rebuildHighlightCache() {
    const chordDescr = root.activeChord && root.activeChord.chordDescriptor ? root.activeChord.chordDescriptor : null;
    const scale = root.activeScale && root.activeScale.scale ? root.activeScale.scale : null;
    root._highlightCache = ChordHighlighter.highlightColors(root.palette.highlight, chordDescr, scale, root.highlightMode, 0.8);
  }

  Component.onCompleted: root._rebuildHighlightCache()
  onActiveChordChanged: root._rebuildHighlightCache()
  onActiveScaleChanged: root._rebuildHighlightCache()
  onHighlightModeChanged: root._rebuildHighlightCache()
  onPaletteChanged: root._rebuildHighlightCache()

  // Re-read live descriptor state on chord/scale content changes so that
  // in-place descriptor edits (which leave the activeChord pointer unchanged)
  // still refresh the highlight colours.
  Connections {
    function onContentChanged() {
      root._rebuildHighlightCache();
    }

    target: root.chordTrack?.chordClips ?? null
  }
  Connections {
    function onContentChanged() {
      root._rebuildHighlightCache();
    }

    target: root.chordTrack?.scaleObjects ?? null
  }

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

  Connections {
    function onActiveNotesChanged() {
      for (let i = 0; i < repeater.count; ++i) {
        const item = repeater.itemAt(i);
        if (item)
          item.isPressed = root.noteActivityProvider.isNoteActive(item.midiNote);
      }
    }

    target: root.noteActivityProvider
  }

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
          visible: keyItem.midiNote < root.endNote && root.midiEditor.isWhiteKey(keyItem.midiNote) && root.midiEditor.isNextKeyWhite(keyItem.midiNote)
          width: parent.width
        }

        // White key background
        Rectangle {
          anchors {
            bottom: parent.bottom
            left: parent.left
            top: topBorder.visible ? topBorder.bottom : parent.top
          }
          border.width: 0
          color: parent.isPressed && root.midiEditor.isWhiteKey(keyItem.midiNote) ? root.whiteKeyPressedColor : "white"
          width: parent.width
        }

        // Highlight overlay (on top of both white and black keys)
        Rectangle {
          id: highlightOverlay

          anchors.fill: parent
          border.width: 0
          color: root._highlightCache[keyItem.midiNote % 12]
          visible: color.a > 0
          z: 3
        }

        // Black key
        Rectangle {
          border.width: 0
          bottomRightRadius: root.blackKeyRadius
          color: parent.isPressed ? root.blackKeyPressedColor : root.blackKeyColor
          height: parent.height
          topRightRadius: root.blackKeyRadius
          visible: root.midiEditor.isBlackKey(keyItem.midiNote)
          width: root.blackKeyWidth
          z: 1

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
          visible: root.midiEditor.isWhiteKey(keyItem.midiNote) && (keyItem.midiNote % 12 === 0)
          z: 4

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
      const idx = Math.floor(yPos / root.keyHeight);
      if (idx < 0 || idx >= repeater.count)
        return;

      currentNote = root.endNote - idx;
      if (currentNote !== lastValidNote) {
        if (lastValidNote !== -1) {
          root.noteReleased(lastValidNote);
        }
        root.notePressed(currentNote);
        lastValidNote = currentNote;
      }
    }

    function handleNoteRelease() {
      if (lastValidNote !== -1) {
        root.noteReleased(lastValidNote);
        lastValidNote = -1;
      }
      currentNote = -1;
    }

    acceptedButtons: Qt.LeftButton | Qt.RightButton
    anchors.fill: parent
    preventStealing: true
    z: 4

    onPositionChanged: {
      if (pressed)
        handleNotePress(mouseY);
    }
    onPressed: handleNotePress(mouseY)
    onReleased: handleNoteRelease()
  }
}
