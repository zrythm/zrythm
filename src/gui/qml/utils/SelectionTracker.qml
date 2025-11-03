// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQml

QtObject {
  id: root

  property bool isSelected: {
    if (root.selectionModel && root.selectionModel.model) {
      root.selectionModel.selection; // binding
      return root.selectionModel.isSelected(root.unifiedModelIndex);
    }
    else {
      return false;
    }
  }
  required property var unifiedModelIndex
  required property ItemSelectionModel selectionModel
}
