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
