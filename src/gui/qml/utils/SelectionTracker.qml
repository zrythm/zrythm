// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQml

// A helper that binds to a selection model and listens for selection changes and updates the selection state of the given model index
QtObject {
  id: root

  property bool isSelected: {
    if (root.selectionModel && root.selectionModel.model) {
      root.selectionModel.selection; // binding
      return root.selectionModel.isSelected(root.modelIndex);
    }
    else {
      return false;
    }
  }
  required property var modelIndex
  required property ItemSelectionModel selectionModel
}
