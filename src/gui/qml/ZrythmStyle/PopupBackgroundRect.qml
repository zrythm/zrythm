// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Effects
import ZrythmStyle 1.0

Rectangle {
  id: root

  color: Style.colorPalette.button
  implicitHeight: Style.buttonHeight
  implicitWidth: Style.buttonHeight // 200
  layer.enabled: true
  radius: Style.textFieldRadius

  layer.effect: DropShadowEffect {
  }

  border {
    color: Style.backgroundAppendColor
    width: 1
  }
}
