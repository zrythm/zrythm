// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

Button {
  id: root

  checkable: true
  text: "S"

  palette {
    accent: Style.soloGreenColor
  }

  ToolTip {
    text: qsTr("Solo")
  }
}
