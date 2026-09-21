// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.Dialog {
  id: control

  implicitHeight: Math.max(
    implicitBackgroundHeight + topInset + bottomInset,
    contentHeight + topPadding + bottomPadding
      + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
      + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))
  implicitWidth: Math.max(
    implicitBackgroundWidth + leftInset + rightInset,
    contentWidth + leftPadding + rightPadding, implicitHeaderWidth,
    implicitFooterWidth)
  padding: 12

  T.Overlay.modal: Rectangle {
    color: Color.transparent(control.palette.shadow, 0.5)
  }
  T.Overlay.modeless: Rectangle {
    color: Color.transparent(control.palette.shadow, 0.12)
  }

  // Dialogs render as native windows (popupType: Window), so the OS
  // provides the title bar, frame and shadow; the background is the
  // flat client area inside that frame.
  background: Rectangle {
    color: control.palette.window
  }

  footer: DialogButtonBox {
    visible: count > 0
  }
  header: Label {
    elide: Label.ElideRight
    font.bold: true
    padding: 12
    text: control.title
    visible: parent?.parent === Overlay.overlay && control.title
  }
}
