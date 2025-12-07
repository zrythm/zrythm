// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

GridLayout {
  id: root

  readonly property Project project: projectUiState.project
  required property ProjectUiState projectUiState
  required property var region
}
