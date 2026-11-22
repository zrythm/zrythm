// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Effects
import ZrythmStyle 1.0

Rectangle {
  id: root

  color: ZrythmTheme.colorPalette.button
  implicitHeight: ZrythmTheme.buttonHeight
  implicitWidth: ZrythmTheme.buttonHeight // 200
  layer.enabled: true
  radius: ZrythmTheme.textFieldRadius

  layer.effect: DropShadowEffect {
  }

  border {
    color: ZrythmTheme.backgroundAppendColor
    width: 1
  }
}
