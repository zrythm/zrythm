// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Effects
import ZrythmStyle 1.0

Rectangle {
    id: root
    implicitWidth: Style.buttonHeight // 200
    implicitHeight: Style.buttonHeight
    color: palette.button
    radius: Style.buttonRadius
    layer.enabled: true

    border {
        color: Style.backgroundAppendColor
        width: 1
    }

    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowHorizontalOffset: 3
        shadowVerticalOffset: 3
        shadowColor: root.palette.shadow
        shadowBlur: 0.8
    }

}
