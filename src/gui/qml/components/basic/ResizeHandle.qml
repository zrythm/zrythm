// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmStyle

Rectangle {
  id: resizeHandleRect

  required property var resizeTarget

  signal resizingChanged(bool active)

  color: "transparent"
  height: 6

  anchors {
    bottom: parent.bottom
    left: parent.left
    right: parent.right
  }

  Rectangle {
    anchors.centerIn: parent
    color: Qt.alpha(Style.backgroundAppendColor, 0.2)
    height: 2
    radius: 2
    width: 24
  }

  DragHandler {
    id: resizer

    property real startHeight: 0

    grabPermissions: PointerHandler.CanTakeOverFromItems | PointerHandler.CanTakeOverFromHandlersOfSameType
    xAxis.enabled: false
    yAxis.enabled: true

    onActiveChanged: {
      resizeHandleRect.resizingChanged(active);
      if (active)
        resizer.startHeight = resizeHandleRect.resizeTarget.height;
    }
    onTranslationChanged: {
      if (active) {
        let newHeight = Math.max(resizer.startHeight + translation.y, 24);
        resizeHandleRect.resizeTarget.height = newHeight;
      }
    }
  }

  MouseArea {
    acceptedButtons: Qt.NoButton // Pass through mouse events to the DragHandler
    anchors.fill: parent
    cursorShape: Qt.SizeVerCursor
  }
}
