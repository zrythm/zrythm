// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.MenuBar {
  id: control

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)

  background: Rectangle {
    color: Style.backgroundColor
    implicitHeight: Style.buttonHeight
  }
  contentItem: Row {
    spacing: control.spacing

    Repeater {
      model: control.contentModel
    }
  }
  delegate: MenuBarItem {
  }
}
