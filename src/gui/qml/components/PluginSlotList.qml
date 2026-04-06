// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound
import QtQuick
import Zrythm

ListView {
  id: root

  required property PluginGroup pluginGroup
  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  readonly property PluginSelectionModel pluginSelectionModel: internalSelectionModel
  required property Track track
  required property TrackSelectionModel trackSelectionModel

  signal pluginClicked(Plugin plugin)

  implicitHeight: contentHeight
  interactive: false
  model: pluginGroup

  delegate: PluginSlotView {
    pluginGroup: root.pluginGroup
    pluginImporter: root.pluginImporter
    pluginModelIndex: pluginSelectionModel.getModelIndex(index)
    pluginOperator: root.pluginOperator
    pluginSelectionModel: root.pluginSelectionModel
    track: root.track
    trackSelectionModel: root.trackSelectionModel

    onPluginClicked: function(plugin: Plugin) {
      root.pluginClicked(plugin);
    }
  }
  footer: DropAreaBase {
    id: dropArea

    height: 24
    text: qsTr("Drop plugins here")
    width: ListView.view.width

    onDataDropped: (drop, dragSource) => {
      const pluginSrc = dragSource as PluginDragItem;
      if (pluginSrc && pluginSrc.selectedPlugins.length > 0) {
        root.pluginOperator.movePlugins(
          pluginSrc.selectedPlugins,
          pluginSrc.sourceGroup,
          pluginSrc.sourceTrack,
          root.pluginGroup,
          root.track,
          -1,
        );
        return;
      }
      const descSrc = dragSource as DescriptorDragItem;
      if (descSrc && descSrc.descriptor) {
        root.pluginImporter.importPluginToGroup(descSrc.descriptor, root.pluginGroup);
      }
    }
  }

  PluginSelectionModel {
    id: internalSelectionModel

    model: root.pluginGroup
  }
}
