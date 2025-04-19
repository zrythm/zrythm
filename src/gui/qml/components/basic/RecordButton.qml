// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Button {
    id: root
    checkable: true

    icon.source: ResourceManager.getIconUrl("zrythm-dark", "record.svg")

    palette {
        buttonText: Style.dangerColor
        accent: Style.dangerColor
    }

    ToolTip {
        text: qsTr("Record")
    }
}