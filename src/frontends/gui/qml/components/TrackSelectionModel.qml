// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

ItemSelectionModel {
  id: root

  function getModelIndex(trackIndex: int): var {
    return model.index(trackIndex, 0);
  }

  function getTrackFromModelIndex(sourceIndex: var): Track {
    return sourceIndex.data(TrackCollection.TrackPtrRole);
  }

  function getUuidStringFromModelIndex(sourceIndex: var): string {
    return sourceIndex.data(TrackCollection.TrackUuidStringRole);
  }

  // Returns the current track's UUID, or the first selected track's when the
  // current index is unset.
  function currentOrFirstSelectedUuidString(): string {
    let index = currentIndex;
    if (!(index && index.valid))
      index = selectedIndexes.length > 0 ? selectedIndexes[0] : null;
    if (!(index && index.valid))
      return "";
    return getUuidStringFromModelIndex(index);
  }

  function selectSingleTrack(trackModelIndex: var) {
    if (!root.isSelected(trackModelIndex)) {
      root.clear();
    }
    root.setCurrentIndex(trackModelIndex, ItemSelectionModel.Select);
  }

  // Selects the tracks identified by uuidStrings (e.g. the return value
  // of a paste or duplicate)
  function selectTracksByUuidStrings(uuidStrings: list<string>) {
    root.clear();
    let firstIndex = null;
    for (let row = 0; row < model.rowCount(); row++) {
      const rowIdx = model.index(row, 0);
      const rowUuidString = rowIdx.data(TrackCollection.TrackUuidStringRole);
      if (uuidStrings.includes(rowUuidString)) {
        root.select(rowIdx, ItemSelectionModel.Select);
        if (firstIndex === null)
          firstIndex = rowIdx;
      }
    }
    if (firstIndex !== null)
      root.setCurrentIndex(firstIndex, ItemSelectionModel.NoUpdate);
  }
}
