// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound
import QtQuick
import Zrythm

ListView {
  id: root

  required property PluginGroup pluginGroup
  required property PluginImporter pluginImporter
  readonly property PluginSelectionModel pluginSelectionModel: internalSelectionModel
  required property Track track
  required property TrackSelectionModel trackSelectionModel

  implicitHeight: contentHeight
  interactive: false
  model: pluginGroup

  delegate: PluginSlotView {
    required property int index

    pluginModelIndex: pluginSelectionModel.getModelIndex(index)
    pluginSelectionModel: root.pluginSelectionModel
    track: root.track
    trackSelectionModel: root.trackSelectionModel
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

  PluginSelectionModel {
    id: internalSelectionModel

    model: root.pluginGroup
  }
}
