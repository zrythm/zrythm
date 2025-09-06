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
    if (selectionModel && selectionModel.model) {
      isSelected = selectionModel.isSelected(modelIndex);
    }
  }

  // Initialize and update when index changes
  Component.onCompleted: updateSelection()
  onModelIndexChanged: updateSelection()
  onSelectionModelChanged: {
    if (selectionModel) {
      selectionModel.selectionChanged.connect(root.updateSelection);
    }
  }
}
