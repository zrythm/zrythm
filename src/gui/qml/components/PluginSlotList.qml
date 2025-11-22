// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound
import QtQuick
import Zrythm

ListView {
  id: root

  required property PluginGroup pluginGroup
  required property PluginImporter pluginImporter
  required property Track track

  implicitHeight: contentHeight
  interactive: false
  model: pluginGroup

  delegate: PluginSlotView {
    track: root.track
  }
  footer: DropAreaBase {
    id: dropArea

    height: 24
    text: qsTr("Drop plugins here")
    width: ListView.view.width

    onDataDropped: (drop, dragSource) => {
      console.log("Drop on plugin list");

      if (dragSource.descriptor as PluginDescriptor) {
        root.pluginImporter.importPluginToGroup(dragSource.descriptor, root.pluginGroup);
      }
    }
  }
}
