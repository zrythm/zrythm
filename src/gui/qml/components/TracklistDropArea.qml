// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

Rectangle {
  id: root

  readonly property bool dragInProgress: dropArea.containsDrag

  signal filesDropped(var filePaths)
  signal pluginDescriptorDropped(PluginDescriptor descriptor)

  // Visual feedback for drop area
  border.color: dragInProgress ? "#5a5a5a" : "transparent"
  border.width: 2
  color: dragInProgress ? "#3a3a3a" : "transparent"
  implicitHeight: 60
  implicitWidth: 60
  opacity: dragInProgress ? 0.7 : 1.0
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
      console.log("Drop on tracklist");

      // Emit a signal with the dropped files
      let uniqueFilePaths = DragUtils.getUniqueFilePaths(drop);
      if (uniqueFilePaths.length > 0) {
        root.filesDropped(uniqueFilePaths);
      } else if (drag.source.descriptor as PluginDescriptor) {
        root.pluginDescriptorDropped(drag.source.descriptor);
      } else if (drop.formats.indexOf("application/x-plugin-descriptor") !== -1) {
        const descriptorSerialized = drop.getDataAsString("application/x-plugin-descriptor");
        console.log(descriptorSerialized);
      }
    }
  }

  Text {
    anchors.centerIn: parent
    color: "#cccccc"
    text: qsTr("Drop files and plugins here")
    visible: root.dragInProgress
  }
}
