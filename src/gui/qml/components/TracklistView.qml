// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQml.Models
import Zrythm

ListView {
  id: root

  required property AudioEngine audioEngine

  // Drag-and-drop state shared with delegates. Delegate properties are
  // prefixed with "listView" (e.g., listViewDraggedTrack) to distinguish
  // them from these ListView-level properties.
  property var draggedTrack: null
  property var dropTargetFolder: null
  property int dropTargetIndex: -1
  required property bool pinned
  required property TrackCollectionOperator trackCollectionOperator
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  Layout.fillWidth: true
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  delegateModelAccess: DelegateModel.ReadWrite
  implicitHeight: 200
  implicitWidth: 200

  delegate: TrackView {
    id: trackView

    required property int index

    audioEngine: root.audioEngine
    listViewDraggedTrack: root.draggedTrack
    listViewDropTargetFolder: root.dropTargetFolder
    listViewDropTargetIndex: root.dropTargetIndex
    listViewIsLast: index === ListView.view.count - 1
    listViewPastEndIndex: root.pinned ? ListView.view.count : ListView.view.count + root.tracklist.pinnedTracksCutoff
    trackCollectionOperator: root.trackCollectionOperator
    trackIndex: root.pinned ? index : index + root.tracklist.pinnedTracksCutoff
    trackSelectionModel: root.trackSelectionModel
    tracklist: root.tracklist
    undoStack: root.undoStack
    width: ListView.view.width

    Binding on height {
      value: 0
      when: !trackView.track.visible
    }

    onDropTargetChanged: function (index) {
      root.dropTargetIndex = index;
      root.dropTargetFolder = null;
    }
    onDropTargetFolderChanged: function (track, index) {
      root.dropTargetFolder = track;
      root.dropTargetIndex = index;
    }
    onTrackDragEnded: {
      if (root.dropTargetIndex >= 0 && root.draggedTrack !== null) {
        // Gather all selected tracks, sorted by current position
        const selectedIndexes = root.trackSelectionModel.selectedIndexes;
        let tracksToMove = [];
        for (const idx of selectedIndexes) {
          const t = idx.data(TrackCollection.TrackPtrRole);
          if (t !== null)
            tracksToMove.push({
              track: t,
              pos: idx.row
            });
        }
        if (tracksToMove.length === 0)
          tracksToMove.push({
            track: root.draggedTrack,
            pos: -1
          });

        // Sort by current position
        tracksToMove.sort((a, b) => a.pos - b.pos);

        // Pass the raw drop target index directly - the command handles
        // index adjustment internally.
        const targetPos = root.dropTargetIndex;
        if (targetPos >= 0) {
          const trackList = tracksToMove.map(e => e.track);
          root.trackCollectionOperator.moveTracks(trackList, targetPos, root.dropTargetFolder);
        }
      }
      root.draggedTrack = null;
      root.dropTargetIndex = -1;
      root.dropTargetFolder = null;
    }
    onTrackDragStarted: {
      root.draggedTrack = trackView.track;
    }
  }
  model: SortFilterProxyModel {
    id: proxyModel

    model: root.tracklist.collection

    filters: [
      FunctionFilter {
        function filter(data: TrackRoleData): bool {
          return root.tracklist.shouldBeVisible(data.track) && root.tracklist.isTrackPinned(data.track) === root.pinned;
        }
      }
    ]
  }

  Connections {
    function onPinnedTracksCutoffChanged() {
      proxyModel.invalidate();
    }

    target: root.tracklist
  }

  component TrackRoleData: QtObject {
    property Track track
  }
}
