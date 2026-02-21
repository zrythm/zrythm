// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

Button {
  id: root

  checkable: true
  icon.source: ResourceManager.getIconUrl("zrythm-dark", "record.svg")

  palette {
    accent: Style.dangerColor
    buttonText: Style.dangerColor
  }

  ToolTip {
    text: qsTr("Record")
  }
}
