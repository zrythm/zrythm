// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQml

QtObject {
  id: root

  property bool isSelected: false
  required property var modelIndex
  required property ItemSelectionModel selectionModel

  // Update function
  function updateSelection() {
    if (root.selectionModel && root.selectionModel.model) {
      isSelected = root.selectionModel.isSelected(root.modelIndex);
    }
  }

  // Initialize and update when index changes
  Component.onCompleted: updateSelection()
  Component.onDestruction: {
    if (root.selectionModel) {
      root.selectionModel.selectionChanged.disconnect(root.updateSelection);
    }
  }
  onModelIndexChanged: updateSelection()
  onSelectionModelChanged: {
    if (root.selectionModel) {
      root.selectionModel.selectionChanged.connect(root.updateSelection);
    }
  }
}
