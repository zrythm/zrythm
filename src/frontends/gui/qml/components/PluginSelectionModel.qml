// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

ItemSelectionModel {
  id: root

  function getModelIndex(pluginIndex: int): var {
    return model.index(pluginIndex, 0);
  }

  function getPluginFromModelIndex(sourceIndex: var): Plugin {
    return sourceIndex.data(PluginGroup.DeviceGroupPtrRole);
  }

  function selectSinglePlugin(pluginModelIndex: var) {
    if (!root.isSelected(pluginModelIndex)) {
      root.clear();
    }
    root.setCurrentIndex(pluginModelIndex, ItemSelectionModel.Select);
  }

  // Selects the plugins identified by uuidStrings (e.g. the return value
  // of a paste or duplicate)
  function selectPluginsByUuidStrings(uuidStrings: list<string>) {
    root.clear();
    let firstIndex = null;
    for (let row = 0; row < model.rowCount(); row++) {
      const rowIdx = model.index(row, 0);
      const rowUuidString = rowIdx.data(PluginGroup.PluginUuidStringRole);
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
