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
    accent: ZrythmTheme.recordColor

    // lighter record red for readable text
    buttonText: ZrythmTheme.darkMode ? "#FF5F9E" : ZrythmTheme.recordColor
  }

  ToolTip {
    text: qsTr("Record")
  }
}
