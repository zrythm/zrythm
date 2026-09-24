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
  property Track draggedTrack: null
  property Track dropTargetFolder: null
  property int dropTargetIndex: -1
  required property PortObservationManager portObservationManager
  required property bool pinned
  required property TrackCollectionOperator trackCollectionOperator
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  readonly property ClipboardContext clipboardContext: ClipboardContext {
    canCopy: root.trackSelectionModel.hasSelection
    canPaste: root.trackCollectionOperator.canPasteTracks

    onCopyRequested: root.trackCollectionOperator.copyTracks(root.selectedTracks())
    onCutRequested: root.trackCollectionOperator.cutTracks(root.selectedTracks())
    onPasteRequested: {
      const newUuidStrings = root.trackCollectionOperator.pasteTracks(root.pasteTargetPosition());
      if (newUuidStrings.length > 0)
        root.trackSelectionModel.selectTracksByUuidStrings(newUuidStrings);
    }
    onDuplicateRequested: {
      const newUuidStrings = root.trackCollectionOperator.duplicateTracks(root.selectedTracks());
      if (newUuidStrings.length > 0)
        root.trackSelectionModel.selectTracksByUuidStrings(newUuidStrings);
    }
  }

  function selectedTracks(): list<Track> {
    const tracks = [];
    for (const idx of root.trackSelectionModel.selectedIndexes) {
      const track = root.trackSelectionModel.getTrackFromModelIndex(idx);
      if (track !== null)
        tracks.push(track);
    }
    return tracks;
  }

  // Position the first pasted track ends up at: after the last selected
  // track, or past the end when the selection is empty
  function pasteTargetPosition(): int {
    let targetPosition = -1;
    for (const idx of root.trackSelectionModel.selectedIndexes) {
      if (targetPosition < idx.row + 1)
        targetPosition = idx.row + 1;
    }
    return targetPosition;
  }

  // Passive tap tracking: clicking anywhere in the tracklist focuses
  // it, which makes the window resolve the active clipboard context
  // from here
  TapHandler {
    onTapped: root.forceActiveFocus()
  }

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
    clipboardContext: root.clipboardContext
    listViewDraggedTrack: root.draggedTrack
    listViewDropTargetFolder: root.dropTargetFolder
    listViewDropTargetIndex: root.dropTargetIndex
    listViewIsLast: index === ListView.view.count - 1
    portObservationManager: root.portObservationManager
    trackCollectionOperator: root.trackCollectionOperator
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
