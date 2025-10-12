// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

Rectangle {
  id: root

  signal filesDropped(var filePaths)

  // Visual feedback for drop area
  border.color: dropArea.containsDrag ? "#5a5a5a" : "transparent"
  border.width: 2
  color: dropArea.containsDrag ? "#3a3a3a" : "transparent"
  implicitHeight: 60
  implicitWidth: 60
  opacity: dropArea.containsDrag ? 0.7 : 1.0
  radius: 4

  ContextMenu.menu: Menu {
    MenuItem {
      text: qsTr("Test")

      onTriggered: console.log("Clicked")
    }
  }

  // Add DropArea to handle file drops
  DropArea {
    id: dropArea

    anchors.fill: parent

    // Accept both internal and external file drag formats
    // keys: ["file", "text/plain", "text/uri-list", "application/x-file-path"]

    onDropped: drop => {
      // Handle the dropped file(s)
      console.log("File dropped on tracklist");

      // Emit a signal with the dropped files
      let uniqueFilePaths = DragUtils.getUniqueFilePaths(drop);
      root.filesDropped(uniqueFilePaths);
    }
  }

  Text {
    anchors.centerIn: parent
    color: "#cccccc"
    text: qsTr("Drop files and plugins here")
    visible: dropArea.containsDrag
  }
}
