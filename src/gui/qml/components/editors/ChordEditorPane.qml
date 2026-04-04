// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm

GridLayout {
  id: root

  readonly property Project project: session.project
  required property ProjectSession session
  required property var region
}
