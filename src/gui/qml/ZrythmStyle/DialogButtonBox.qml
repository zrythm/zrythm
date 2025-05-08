// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T

T.DialogButtonBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            (control.count === 1 ? implicitContentWidth * 2 : implicitContentWidth) + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    contentWidth: (contentItem as ListView)?.contentWidth

    spacing: 2
    padding: 12
    alignment: count === 1 ? Qt.AlignRight : undefined

    delegate: Button {
        width: control.count === 1 ? control.availableWidth / 2 : undefined
    }

    contentItem: ListView {
        implicitWidth: contentWidth
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
    }

    background: Rectangle {
        implicitHeight: 40
        x: 1; y: 1
        width: parent.width - 2
        height: parent.height - 2
        color: control.palette.window
    }
}
