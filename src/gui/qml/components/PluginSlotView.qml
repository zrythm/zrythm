// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

Button {
  required property Plugin plugin

  text: plugin.configuration.descriptor.name

  onClicked: plugin.uiVisible = !plugin.uiVisible
}
