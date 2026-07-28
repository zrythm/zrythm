// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick

// Tracks whether a delegate's item is selected in a selection model.
//
// The hasSelection dependency makes the binding re-evaluate on every
// selectionChanged() signal (documented ItemSelectionModel behavior), and
// each evaluation calls modelIndexProvider() to get a fresh model index.
// QModelIndex values must not be cached in QML because they are frozen
// snapshots that go stale when model rows change.
Item {
  id: root

  // Returns a fresh model index for this delegate's item.
  required property var modelIndexProvider // () => QModelIndex
  required property ItemSelectionModel selectionModel
  readonly property bool isSelected: selectionModel.hasSelection && selectionModel.isSelected(modelIndexProvider())
}
