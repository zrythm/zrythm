// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.ToolSeparator {
  id: control

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  padding: vertical ? 6 : 2
  verticalPadding: vertical ? 2 : 6

  contentItem: Rectangle {
    color: control.palette.text
    implicitHeight: control.vertical ? 18 : 1
    implicitWidth: control.vertical ? 1 : 18
    opacity: 0.2
  }
}
