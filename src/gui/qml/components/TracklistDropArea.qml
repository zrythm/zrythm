// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

DropAreaBase {
  id: root

  signal filesDropped(var filePaths)
  signal pluginDescriptorDropped(PluginDescriptor descriptor)

  text: qsTr("Drop files and plugins here")

  onDataDropped: (drop, dragSource) => {
    console.log("Drop on tracklist");

    // Emit a signal with the dropped files
    let uniqueFilePaths = DragUtils.getUniqueFilePaths(drop);
    if (uniqueFilePaths.length > 0) {
      root.filesDropped(uniqueFilePaths);
    } else if (dragSource.descriptor as PluginDescriptor) {
      root.pluginDescriptorDropped(dragSource.descriptor);
    } else if (drop.formats.indexOf("application/x-plugin-descriptor") !== -1) {
      const descriptorSerialized = drop.getDataAsString("application/x-plugin-descriptor");
      console.log(descriptorSerialized);
    }
  }
}
