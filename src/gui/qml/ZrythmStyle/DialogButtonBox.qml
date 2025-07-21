// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T

T.DialogButtonBox {
  id: control

  alignment: count === 1 ? Qt.AlignRight : undefined
  contentWidth: (contentItem as ListView)?.contentWidth
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, (control.count === 1 ? implicitContentWidth * 2 : implicitContentWidth) + leftPadding + rightPadding)
  padding: 12
  spacing: 2

  background: Rectangle {
    color: control.palette.window
    height: parent.height - 2
    implicitHeight: 40
    width: parent.width - 2
    x: 1
    y: 1
  }
  contentItem: ListView {
    boundsBehavior: Flickable.StopAtBounds
    implicitWidth: contentWidth
    model: control.contentModel
    orientation: ListView.Horizontal
    snapMode: ListView.SnapToItem
    spacing: control.spacing
  }
  delegate: Button {
    width: control.count === 1 ? control.availableWidth / 2 : undefined
  }
}
