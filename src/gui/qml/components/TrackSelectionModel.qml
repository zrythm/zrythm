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
}
