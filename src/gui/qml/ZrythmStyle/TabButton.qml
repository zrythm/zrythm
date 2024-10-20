// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 4
    spacing: 4

    icon.width: Style.buttonHeight - 2 * padding
    // icon.height: 24
    icon.color: checked ? control.palette.windowText : control.palette.brightText

   font {
        pointSize: Style.fontPointSize
        weight: Font.Medium
    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: control.font
        color: control.checked ? control.palette.windowText : control.palette.brightText
    }

    background: Rectangle {
        implicitHeight: Style.buttonHeight
        radius: Style.buttonRadius
        color: Color.blend(control.checked ? control.palette.window : control.palette.dark,
                                             control.palette.mid, control.down ? 0.5 : 0.0)
    }
}
