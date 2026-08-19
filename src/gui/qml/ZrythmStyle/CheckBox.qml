// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle

T.CheckBox {
  id: control

  font: ZrythmTheme.semiBoldTextFont
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  padding: 4
  spacing: 6

  contentItem: IconLabel {
    alignment: Qt.AlignLeft
    color: control.palette.text
    display: control.display
    font: control.font
    icon: control.icon
    leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
    mirrored: control.mirrored
    rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0
    spacing: control.spacing
    text: control.text
  }
  indicator: CheckIndicator {
    checked: control.checked
    down: control.down
    hovered: control.hovered
    visualFocus: control.visualFocus
    x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
    y: control.topPadding + (control.availableHeight - height) / 2
  }
}
