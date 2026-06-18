// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm

Column {
  id: root

  required property ChordRowListModel chordRowModel
  required property ChordTrack chordTrack
  required property int rowHeight
  required property UndoStack undoStack

  // Emitted when a row header is double-clicked (batch edit request).
  signal rowEditRequested (int row)

  // Emitted when the "+" new-chord row is clicked.
  signal newChordRequested ()

  spacing: 0

  Repeater {
    model: root.chordRowModel

    delegate: AbstractButton {
      id: rowLabel

      required property string chordName
      required property ChordDescriptor chordDescriptor
      required property int chordObjectCount
      required property int index

      height: root.rowHeight
      width: root.width

      background: Rectangle {
        border.color: root.palette.mid
        border.width: 1
        color: rowLabel.down ? root.palette.highlight : root.palette.button
      }
      contentItem: Row {
        spacing: 4

        Label {
          color: rowLabel.down ? root.palette.highlightedText : root.palette.text
          font.bold: true
          text: rowLabel.chordName
          verticalAlignment: Text.AlignVCenter
        }

        Label {
          color: root.palette.placeholderText
          font.pixelSize: 10
          text: "×" + rowLabel.chordObjectCount
          verticalAlignment: Text.AlignVCenter
          visible: rowLabel.chordObjectCount > 1
        }
      }

      onPressed: {
        if (root.chordTrack && rowLabel.chordDescriptor)
          root.chordTrack.auditionChord (rowLabel.chordDescriptor, true);
      }
      onReleased: {
        if (root.chordTrack && rowLabel.chordDescriptor)
          root.chordTrack.auditionChord (rowLabel.chordDescriptor, false);
      }
      onDoubleClicked: {
        if (root.chordTrack && rowLabel.chordDescriptor)
          root.chordTrack.auditionChord (rowLabel.chordDescriptor, false);
        root.rowEditRequested (rowLabel.index);
      }
    }
  }

  // Trailing "+" new-chord row.
  AbstractButton {
    height: root.rowHeight
    width: root.width

    background: Rectangle {
      border.color: root.palette.mid
      border.width: 1
      color: "transparent"
    }
    contentItem: Label {
      color: root.palette.mid
      font.pixelSize: 18
      horizontalAlignment: Text.AlignHCenter
      text: "+"
      verticalAlignment: Text.AlignVCenter
    }

    onClicked: root.newChordRequested ()
  }
}
