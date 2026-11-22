// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.TabBar {
  id: control

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
  spacing: 1

  background: Rectangle {
    color: control.palette.window
  }
  contentItem: ListView {
    boundsBehavior: Flickable.StopAtBounds
    currentIndex: control.currentIndex
    flickableDirection: Flickable.AutoFlickIfNeeded
    highlightMoveDuration: 0
    highlightRangeMode: ListView.ApplyRange
    model: control.contentModel
    orientation: ListView.Horizontal
    preferredHighlightBegin: 40
    preferredHighlightEnd: width - 40
    snapMode: ListView.SnapToItem
    spacing: control.spacing
  }
}
