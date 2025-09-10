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

      // Extract file paths from different MIME types
      var filePaths = [];

      // Handle text/plain (internal drags and some external apps)
      if (drop.hasText && drop.text) {
        console.log("Text drop:", drop.text);
        const lines = QmlUtils.splitTextLines(drop.text);
        for (const line of lines) {
          filePaths.push(line);
        }
      }

      // Handle text/uri-list (standard for external file browsers)
      if (drop.hasUrls) {
        console.log("URLs dropped:", drop.urls.length);
        for (const url of drop.urls) {
          // Convert URLs to local file paths
          filePaths.push(QmlUtils.toPathString(url));
        }
      }

      // Handle application/x-file-path (internal format)
      if (drop.formats.indexOf("application/x-file-path") !== -1) {
        const filePathData = drop.getDataAsString("application/x-file-path");
        if (filePathData) {
          console.log("Application file path:", filePathData);
          filePaths.push(filePathData);
        }
      }

      // Remove duplicates
      let uniqueFilePaths = QmlUtils.removeDuplicates(filePaths);

      // Log if duplicates were found and removed
      if (filePaths.length !== uniqueFilePaths.length) {
        console.log("Removed duplicates:", filePaths.length - uniqueFilePaths.length);
      }

      // Emit a signal with the dropped files
      if (uniqueFilePaths.length > 0) {
        console.log("Processing dropped files:", uniqueFilePaths);
        root.filesDropped(uniqueFilePaths);
      }
    }
  }

  Text {
    anchors.centerIn: parent
    color: "#cccccc"
    text: qsTr("Drop files and plugins here")
    visible: dropArea.containsDrag
  }
}
