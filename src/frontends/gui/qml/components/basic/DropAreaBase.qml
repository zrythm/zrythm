// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick

Rectangle {
  id: root

  readonly property bool dragInProgress: dropArea.containsDrag
  property alias text: textLabel.text

  signal dataDropped(DragEvent drop, QtObject dragSource)

  // Visual feedback for drop area
  border.color: dragInProgress ? "#5a5a5a" : "transparent"
  border.width: 2
  color: dragInProgress ? "#3a3a3a" : "transparent"
  implicitHeight: 60
  implicitWidth: 60
  opacity: dragInProgress ? 0.7 : 1.0
  radius: 4

  DropArea {
    id: dropArea

    anchors.fill: parent

    onDropped: drop => {
      root.dataDropped(drop, drag.source);
    }
  }

  Text {
    id: textLabel

    anchors.centerIn: parent
    color: "#cccccc"
    text: qsTr("Drop files and plugins here")
    visible: root.dragInProgress
  }
}
