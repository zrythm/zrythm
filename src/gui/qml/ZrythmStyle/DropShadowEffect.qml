// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Effects
import ZrythmStyle 1.0

MultiEffect {
    id: root
    readonly property real shadowOffset: 2
    shadowEnabled: true
    shadowHorizontalOffset: shadowOffset
    shadowVerticalOffset: shadowOffset
    shadowColor: Style.shadowColor
    shadowBlur: 0.6
}
