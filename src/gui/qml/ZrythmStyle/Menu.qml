// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Effects
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.Menu {
    id: control

    palette: Style.colorPalette

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)
    margins: 0
    overlap: 1
    popupType: Popup.Native // auto-fallbacks to Window, then normal

    delegate: MenuItem {
    }

    contentItem: ListView {
        implicitHeight: contentHeight
        model: control.contentModel
        interactive: Window.window ? contentHeight + control.topPadding + control.bottomPadding > control.height : false
        clip: true
        currentIndex: control.currentIndex
        headerPositioning: ListView.OverlayHeader
        footerPositioning: ListView.OverlayFooter

        header: Item {
            height: Style.buttonRadius
        }

        footer: Item {
            height: Style.buttonRadius
        }

        ScrollIndicator.vertical: ScrollIndicator {
        }

    }

    background: Loader {
        sourceComponent: Style.popupBackground
    }

    T.Overlay.modal: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.5)
    }

    T.Overlay.modeless: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.12)
    }

}
