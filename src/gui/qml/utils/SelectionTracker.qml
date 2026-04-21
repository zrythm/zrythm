// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick

// A helper that binds to a selection model and listens for selection changes and updates the selection state of the given model index
Item {
  id: root

  property bool isSelected: false
  required property var modelIndex
  required property ItemSelectionModel selectionModel

  function updateSelection() {
    if (root.selectionModel && root.selectionModel.model)
      root.isSelected = root.selectionModel.isSelected(root.modelIndex);
    else
      root.isSelected = false;
  }

  Component.onCompleted: updateSelection()
  onModelIndexChanged: updateSelection()

  Connections {
    function onSelectionChanged() {
      root.updateSelection();
    }

    target: root.selectionModel
  }
}
