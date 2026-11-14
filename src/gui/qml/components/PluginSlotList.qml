// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import Zrythm

ListView {
  id: root

  readonly property ViewContext viewContext: ViewContext {
    canCopy: root.pluginSelectionModel.hasSelection
    canPaste: root.pluginOperator.canPastePlugins
    canDelete: root.pluginSelectionModel.hasSelection

    onCopyRequested: root.pluginOperator.copyPlugins(root.selectedPlugins())
    onCutRequested: root.pluginOperator.cutPlugins(root.selectedPlugins(), root.pluginGroup, root.track)
    onDuplicateRequested: {
      const duplicatedIds = root.pluginOperator.duplicatePlugins(root.selectedPlugins(), root.pluginGroup);
      if (duplicatedIds.length > 0)
        root.pluginSelectionModel.selectPluginsByUuidStrings(duplicatedIds);
    }
    onPasteRequested: {
      const pastedIds = root.pluginOperator.pastePlugins(root.pluginGroup, -1);
      if (pastedIds.length > 0)
        root.pluginSelectionModel.selectPluginsByUuidStrings(pastedIds);
    }
    onDeleteRequested: root.pluginOperator.removePlugins(root.selectedPlugins(), root.pluginGroup, root.track)
  }
  required property PluginGroup pluginGroup
  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  readonly property PluginSelectionModel pluginSelectionModel: internalSelectionModel
  required property Track track
  required property TrackSelectionModel trackSelectionModel

  signal pluginClicked(Plugin plugin)

  function selectedPlugins(): list<Plugin> {
    const plugins = [];
    for (const idx of root.pluginSelectionModel.selectedIndexes) {
      const plugin = root.pluginSelectionModel.getPluginFromModelIndex(idx);
      if (plugin !== null)
        plugins.push(plugin);
    }
    return plugins;
  }

  implicitHeight: contentHeight
  interactive: false
  model: pluginGroup

  delegate: PluginSlotView {
    dragItem: pluginDragItem
    pluginGroup: root.pluginGroup
    pluginImporter: root.pluginImporter
    pluginOperator: root.pluginOperator
    pluginSelectionModel: root.pluginSelectionModel
    track: root.track
    trackSelectionModel: root.trackSelectionModel
    viewContext: root.viewContext

    onPluginClicked: function (plugin: Plugin) {
      root.pluginClicked(plugin);
    }
  }
  footer: DropAreaBase {
    id: dropArea

    // Reacts to row changes through the view's count property
    readonly property bool groupEmpty: ListView.view.count === 0

    height: groupEmpty ? Math.max(24, pasteButton.implicitHeight) : 20
    text: qsTr("Drop plugins here")
    width: ListView.view.width

    // Visible unfilled slot: the drop target once the group has plugins
    Rectangle {
      anchors.fill: parent
      color: dropArea.dragInProgress ? Qt.alpha(palette.base, 0.8) : Qt.alpha(palette.alternateBase, 0.6)
      radius: ZrythmTheme.textFieldRadius
      visible: !dropArea.groupEmpty

      ColorImage {
        anchors.centerIn: parent
        color: Qt.alpha(palette.windowText, 0.55)
        source: ResourceManager.getIconUrl("zrythm-dark", "add.svg")
        sourceSize: Qt.size(12, 12)
      }
    }

    onDataDropped: (drop, dragSource) => {
      const pluginSrc = dragSource as PluginDragItem;
      if (pluginSrc && pluginSrc.selectedPlugins.length > 0) {
        root.pluginOperator.movePlugins(pluginSrc.selectedPlugins, pluginSrc.sourceGroup, pluginSrc.sourceTrack, root.pluginGroup, root.track, -1);
        return;
      }
      const descSrc = dragSource as DescriptorDragItem;
      if (descSrc && descSrc.descriptor) {
        root.pluginImporter.importPluginToGroup(descSrc.descriptor, root.pluginGroup);
      }
    }

    // Empty-group state: paste the clipboard contents directly, or show
    // a muted label when there is nothing to paste.
    Label {
      anchors.centerIn: parent
      color: ZrythmTheme.placeholderTextColor
      elide: Text.ElideRight
      font: ZrythmTheme.xSmallTextFont
      horizontalAlignment: Text.AlignHCenter
      text: qsTr("Drop or Paste Plugins Here")
      visible: dropArea.groupEmpty && !root.pluginOperator.canPastePlugins && !dropArea.dragInProgress
      width: parent.width
    }

    Button {
      id: pasteButton

      anchors.centerIn: parent
      flat: true
      font: ZrythmTheme.smallTextFont
      text: qsTr("Paste Plugins")
      visible: dropArea.groupEmpty && root.pluginOperator.canPastePlugins && !dropArea.dragInProgress

      onClicked: {
        const pastedIds = root.pluginOperator.pastePlugins(root.pluginGroup, -1);
        if (pastedIds.length > 0)
          root.pluginSelectionModel.selectPluginsByUuidStrings(pastedIds);
      }

      ToolTip {
        text: qsTr("Paste the plugins from the clipboard")
      }
    }
  }

  // Passive tap tracking: clicking anywhere in the slot list focuses
  // it, which makes the window resolve the active view context from
  // here
  TapHandler {
    onTapped: root.forceActiveFocus()
  }

  PluginSelectionModel {
    id: internalSelectionModel

    model: root.pluginGroup
  }

  // Shared drag source for moving plugins out of this list: owned by the
  // list so it survives the destruction of the slot delegate being dragged
  PluginDragItem {
    id: pluginDragItem
  }
}
