// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

Button {
  id: root

  checkable: true
  text: "M"

  ToolTip {
    text: qsTr("Mute")
  }
}
