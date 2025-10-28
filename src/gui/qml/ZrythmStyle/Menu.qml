// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.Menu {
  // header: Item {
  //     height: Style.buttonRadius
  // }
  // footer: Item {
  //     height: Style.buttonRadius
  // }

  id: control

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
  margins: 0
  overlap: 1
  // popupType: Qt.platform.os === "windows" ? Popup.Window : Popup.Native // auto-fallbacks to Window, then normal (Native crashes on Windows)

  T.Overlay.modal: Rectangle {
    color: Color.transparent(control.palette.shadow, 0.5)
  }
  T.Overlay.modeless: Rectangle {
    color: Color.transparent(control.palette.shadow, 0.12)
  }
  background: PopupBackgroundRect {
    implicitWidth: 200
    radius: 0
  }
  contentItem: ListView {
    clip: true
    currentIndex: control.currentIndex
    footerPositioning: ListView.OverlayFooter
    headerPositioning: ListView.OverlayHeader
    implicitHeight: contentHeight
    interactive: Window.window ? contentHeight + control.topPadding + control.bottomPadding > control.height : false
    model: control.contentModel

    ScrollIndicator.vertical: ScrollIndicator {
    }
  }
  delegate: MenuItem {
  }
}
