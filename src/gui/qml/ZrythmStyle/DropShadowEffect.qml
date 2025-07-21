// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Effects
import ZrythmStyle 1.0

MultiEffect {
  id: root

  readonly property real shadowOffset: 2

  shadowBlur: 0.6
  shadowColor: Style.shadowColor
  shadowEnabled: true
  shadowHorizontalOffset: shadowOffset
  shadowVerticalOffset: shadowOffset
}
