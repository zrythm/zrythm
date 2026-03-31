// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQml.Models
import Zrythm

ListView {
  id: root

  required property AudioEngine audioEngine
  required property bool pinned
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property TrackCollectionOperator trackCollectionOperator
  required property UndoStack undoStack

  property var draggedTrack: null
  property int dropTargetIndex: -1

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
    listViewDropTargetIndex: root.dropTargetIndex
    listViewIsLast: index === ListView.view.count - 1
    listViewPastEndIndex: root.pinned ? ListView.view.count : ListView.view.count + root.tracklist.pinnedTracksCutoff
    trackIndex: root.pinned ? index : index + root.tracklist.pinnedTracksCutoff
    trackSelectionModel: root.trackSelectionModel
    tracklist: root.tracklist
    undoStack: root.undoStack
    width: ListView.view.width

    Binding on height {
      value: 0
      when: !trackView.track.visible
    }

    onTrackDragEnded: {
      if (root.dropTargetIndex >= 0 && root.draggedTrack !== null) {
        // Gather all selected tracks, sorted by current position
        const selectedIndexes = root.trackSelectionModel.selectedIndexes;
        let tracksToMove = [];
        for (const idx of selectedIndexes) {
          const t = idx.data(TrackCollection.TrackPtrRole);
          if (t !== null)
            tracksToMove.push({track: t, pos: idx.row});
        }
        if (tracksToMove.length === 0)
          tracksToMove.push({track: root.draggedTrack, pos: -1});

        // Sort by current position
        tracksToMove.sort((a, b) => a.pos - b.pos);

        // Count how many moved tracks are above the drop target - removing
        // them shifts the target position up by that many spots.
        let targetPos = root.dropTargetIndex;
        let aboveCount = 0;
        for (const entry of tracksToMove) {
          if (entry.pos < targetPos)
            aboveCount++;
        }
        targetPos -= aboveCount;

        if (targetPos >= 0) {
          const trackList = tracksToMove.map(e => e.track);
          root.trackCollectionOperator.moveTracks(trackList, targetPos);

          // Re-select the moved tracks at their new positions (remove+insert
          // invalidates model indexes).
          const collection = root.tracklist.collection;
          let first = true;
          for (const track of trackList) {
            for (let row = 0; row < collection.rowCount(); row++) {
              const idx = collection.index(row, 0);
              const t = idx.data(TrackCollection.TrackPtrRole);
              if (t === track) {
                if (first) {
                  root.trackSelectionModel.selectSingleTrack(idx);
                  first = false;
                } else {
                  root.trackSelectionModel.select(idx, ItemSelectionModel.Select);
                }
                break;
              }
            }
          }
        }
      }
      root.draggedTrack = null;
      root.dropTargetIndex = -1;
    }
    onTrackDragStarted: {
      root.draggedTrack = trackView.track;
    }
    onDropTargetChanged: function(index) {
      root.dropTargetIndex = index;
    }
  }
  model: SortFilterProxyModel {
    id: proxyModel

    model: root.tracklist.collection

    filters: [
      FunctionFilter {
        function filter(data: TrackRoleData): bool {
          return root.tracklist.isTrackPinned(data.track) === root.pinned;
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
