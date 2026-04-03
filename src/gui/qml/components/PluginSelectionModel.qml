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
}
