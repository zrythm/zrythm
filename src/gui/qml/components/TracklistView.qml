// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

ListView {
  id: root

  required property AudioEngine audioEngine
  required property bool pinned
  required property TrackSelectionManager trackSelectionManager
  required property Tracklist tracklist
  required property UndoStack undoStack

  Layout.fillWidth: true
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  implicitHeight: 200
  implicitWidth: 200

  delegate: TrackView {
    audioEngine: root.audioEngine
    trackSelectionManager: root.trackSelectionManager
    tracklist: root.tracklist
    undoStack: root.undoStack
    width: ListView.view.width
  }
  model: TrackFilterProxyModel {
    sourceModel: root.tracklist.collection

    Component.onCompleted: {
      addVisibilityFilter(true);
      addPinnedFilter(root.pinned);
    }
  }
}
