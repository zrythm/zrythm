// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma Singleton

import QtQuick
import Zrythm

QtObject {
  function getUniqueFilePaths(drop: DragEvent): var {
    // Extract file paths from different MIME types
    var filePaths = [];

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
        filePaths.push(filePathData);
      }
    }

    // Remove duplicates
    let uniqueFilePaths = QmlUtils.removeDuplicates(filePaths);

    return uniqueFilePaths;
  }
}
