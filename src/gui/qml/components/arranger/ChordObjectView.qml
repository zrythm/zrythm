// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle
import ZrythmGui

ArrangerObjectBaseView {
  id: root

  readonly property ChordObject chordObject: arrangerObject as ChordObject
  property ChordTrack chordTrack: null
  required property ArrangerObjectCreator objectCreator

  onObjectDoubleClicked: {
    editDialogLoader.active = true;
    const dialog = editDialogLoader.item as ChordSelectorDialog;
    dialog.applyFromDescriptor(root.chordObject.chordDescriptor);
    dialog.open();
  }

  Rectangle {
    anchors.fill: parent
    border.color: Qt.darker(root.objectColor, 1.3)
    border.width: root.isSelected ? 2 : 0
    color: root.objectColor
    radius: ZrythmTheme.toolButtonRadius
  }

  Label {
    anchors.fill: parent
    anchors.leftMargin: 6
    color: root.palette.highlightedText
    elide: Text.ElideRight
    font: root.font
    horizontalAlignment: Text.AlignLeft
    text: root.chordObject?.chordDescriptor?.displayName ?? ""
    verticalAlignment: Text.AlignVCenter
  }

  // Dialog created on demand (only when the object is double-clicked) and
  // released on close, so dense regions don't keep N dialogs alive.
  Loader {
    id: editDialogLoader

    active: false

    sourceComponent: ChordSelectorDialog {
      id: editDialog

      chordTrack: root.chordTrack

      onAccepted: {
        root.objectCreator.editChordObjectsDescriptor([root.chordObject], editDialog.tempRootNote, editDialog.tempChordType, editDialog.tempChordAccent, editDialog.tempHasBass, editDialog.tempHasBass ? editDialog.tempBassNote : MusicalScale.MusicalNote.C, editDialog.tempInversion);
      }
      onClosed: editDialogLoader.active = false
    }
  }
}
