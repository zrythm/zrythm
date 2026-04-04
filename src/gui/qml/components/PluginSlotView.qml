// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

Control {
  id: root

  required property var deviceGroupOrPlugin
  property bool down: false

  // Drop handling
  property bool dropHovered: false
  required property int index
  readonly property bool isCurrentTrack: {
    const currentIdx = root.trackSelectionModel.currentIndex;
    return currentIdx && currentIdx.valid ? currentIdx.data(TrackCollection.TrackPtrRole) === root.track : false;
  }
  readonly property Plugin plugin: deviceGroupOrPlugin as Plugin
  readonly property bool pluginEnabled: root.plugin && root.plugin.bypassParameter.baseValue < 0.5
  required property PluginGroup pluginGroup
  required property PluginImporter pluginImporter
  required property var pluginModelIndex
  required property PluginOperator pluginOperator
  required property PluginSelectionModel pluginSelectionModel
  required property Track track
  required property TrackSelectionModel trackSelectionModel

  function selectCurrentTrack() {
    if (!root.isCurrentTrack) {
      const trackModel = root.trackSelectionModel.model;
      const count = trackModel.rowCount();
      for (let i = 0; i < count; ++i) {
        if (trackModel.index(i, 0).data(TrackCollection.TrackPtrRole) === root.track) {
          root.trackSelectionModel.selectSingleTrack(trackModel.index(i, 0));
          break;
        }
      }
    }
  }

  implicitHeight: 20
  implicitWidth: 48
  width: parent ? parent.width : 200

  // Context menu
  ContextMenu.menu: Menu {
    id: contextMenu

    MenuItem {
      enabled: root.plugin !== null
      text: root.plugin ? qsTr("Remove Plugin") : qsTr("Add Plugin")

      onTriggered: {
        if (root.plugin && root.track && root.track.channel) {
          console.log("remove plugin");
          // root.track.channel.removePlugin(root.slotIndex, root.slotType);
        }
      }
    }

    MenuItem {
      enabled: root.plugin !== null
      text: qsTr("Show Plugin UI")

      onTriggered: {
        if (root.plugin) {
          root.plugin.uiVisible = true;
        }
      }
    }

    MenuSeparator {
      visible: root.plugin !== null
    }

    MenuItem {
      text: qsTr("Properties")

      onTriggered:
      // Open plugin properties dialog
      {}
    }
  }

  SelectionTracker {
    id: selectionTracker

    modelIndex: root.pluginModelIndex
    selectionModel: root.pluginSelectionModel
  }

  TapHandler {
    acceptedButtons: Qt.LeftButton
    acceptedModifiers: Qt.NoModifier

    onTapped: (eventPoint, button) => {
      if (root.plugin) {
        root.selectCurrentTrack();
        root.pluginSelectionModel.selectSinglePlugin(root.pluginModelIndex);
      }
    }
  }

  TapHandler {
    acceptedButtons: Qt.LeftButton
    acceptedModifiers: Qt.ControlModifier

    onTapped: (eventPoint, button) => {
      if (root.plugin) {
        root.selectCurrentTrack();
        root.pluginSelectionModel.select(root.pluginModelIndex, ItemSelectionModel.Toggle);
        root.pluginSelectionModel.setCurrentIndex(root.pluginModelIndex, ItemSelectionModel.NoUpdate);
      }
    }
  }

  TapHandler {
    acceptedButtons: Qt.LeftButton
    acceptedModifiers: Qt.ShiftModifier

    onTapped: (eventPoint, button) => {
      if (root.plugin) {
        root.selectCurrentTrack();
        const currentIdx = root.pluginSelectionModel.currentIndex;
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, root.pluginModelIndex.row);
          const bottom = Math.max(currentIdx.row, root.pluginModelIndex.row);
          const sel = QmlUtils.createRangeSelection(root.pluginSelectionModel.model, top, bottom);
          root.pluginSelectionModel.select(sel, ItemSelectionModel.ClearAndSelect);
        } else {
          root.pluginSelectionModel.selectSinglePlugin(root.pluginModelIndex);
        }
      }
    }
  }

  TapHandler {
    acceptedButtons: Qt.LeftButton
    acceptedModifiers: Qt.ControlModifier | Qt.ShiftModifier

    onTapped: (eventPoint, button) => {
      if (root.plugin) {
        root.selectCurrentTrack();
        const currentIdx = root.pluginSelectionModel.currentIndex;
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, root.pluginModelIndex.row);
          const bottom = Math.max(currentIdx.row, root.pluginModelIndex.row);
          const sel = QmlUtils.createRangeSelection(root.pluginSelectionModel.model, top, bottom);
          root.pluginSelectionModel.select(sel, ItemSelectionModel.Select);
        } else {
          root.pluginSelectionModel.selectSinglePlugin(root.pluginModelIndex);
        }
      }
    }
  }

  TapHandler {
    acceptedButtons: Qt.RightButton

    onTapped: (eventPoint, button) => {
      contextMenu.popup();
    }
  }

  TapHandler {
    onDoubleTapped: (eventPoint, button) => {
      if (root.plugin) {
        root.plugin.uiVisible = !root.plugin.uiVisible;
      }
    }
  }

  // Drag support for reordering/moving plugins
  DragHandler {
    id: pluginDragHandler

    target: null

    onActiveChanged: {
      if (active && root.plugin) {
        // Capture selected plugins at drag start
        const plugins = [];
        const groupModel = root.pluginGroup;
        for (let i = 0; i < groupModel.rowCount(); ++i) {
          const idx = root.pluginSelectionModel.getModelIndex(i);
          if (root.pluginSelectionModel.isSelected(idx)) {
            const pl = root.pluginSelectionModel.getPluginFromModelIndex(idx);
            if (pl)
              plugins.push(pl);
          }
        }
        if (plugins.length === 0)
          plugins.push(root.plugin);

        dragItem.selectedPlugins = plugins;
        dragItem.sourceGroup = root.pluginGroup;
        dragItem.sourceTrack = root.track;
        dragItem.Drag.active = true;
      } else {
        dragItem.Drag.active = false;
      }
    }
  }

  PluginDragItem {
    id: dragItem
  }

  DropArea {
    id: dropArea

    anchors.fill: parent

    onDropped: drop => {
      root.dropHovered = false;
      const pluginSrc = drop.source as PluginDragItem;
      if (pluginSrc && pluginSrc.selectedPlugins.length > 0) {
        root.pluginOperator.movePlugins(pluginSrc.selectedPlugins, pluginSrc.sourceGroup, pluginSrc.sourceTrack, root.pluginGroup, root.track, root.index);
      }
      const descSrc = drop.source as DescriptorDragItem;
      if (descSrc && descSrc.descriptor) {
        root.pluginImporter.importPluginToGroup(descSrc.descriptor, root.pluginGroup, root.index);
      }
    }
    onEntered: drag => {
      const src = drag.source;
      root.dropHovered = (src as PluginDragItem) !== null || (src as DescriptorDragItem) !== null;
    }
    onExited: {
      root.dropHovered = false;
    }
  }

  Rectangle {
    id: bgRect

    readonly property color baseColor: {
      let c = root.pluginEnabled ? palette.base : palette.window;
      if (root.plugin && root.plugin.uiVisible) {
        c = Style.getColorBlendedTowardsContrast(c);
      }
      return c;
    }

    anchors.fill: parent
    border.color: {
      if (root.isCurrentTrack && selectionTracker.isSelected)
        return palette.highlight;
      return palette.alternateBase;
    }
    border.width: 1
    color: Style.adjustColorForHoverOrVisualFocusOrDown(baseColor, root.hovered, root.visualFocus, root.down)
    radius: 6

    // Drop insert indicator: top border only
    Rectangle {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top
      color: palette.highlight
      height: 2
      visible: root.dropHovered
    }
  }

  RowLayout {
    anchors.fill: parent
    spacing: 4

    // Activate button
    RoundButton {
      id: activateButton

      Layout.alignment: Qt.AlignVCenter
      Layout.leftMargin: 4
      Layout.preferredHeight: 8
      Layout.preferredWidth: 8
      checkable: true
      checked: root.pluginEnabled
      radius: 16

      onClicked: {
        if (root.plugin) {
          root.plugin.bypassParameter.baseValue = (root.pluginEnabled) ? 1.0 : 0.0;
        }
      }
    }

    // Text display
    Text {
      id: slotText

      Layout.alignment: Qt.AlignVCenter
      Layout.fillWidth: true
      Layout.rightMargin: 4
      color: {
        let c;
        if (root.plugin && root.plugin.instantiationStatus === Plugin.Failed) {
          c = Style.dangerColor;
        } else {
          c = palette.text;
        }
        if (!root.pluginEnabled) {
          return Style.getColorBlendedTowardsContrast(c);
        }
        return c;
      }
      elide: Text.ElideRight
      text: {
        if (root.plugin) {
          if (root.plugin.instantiationStatus === Plugin.Failed) {
            return "(!) " + root.plugin.configuration.descriptor.name;
          } else {
            return root.plugin.configuration.descriptor.name;
          }
        } else {
          return "";
        }
      }
    }
  }

  ToolTip {
    text: root.plugin ? root.plugin.configuration.descriptor.name : ""
  }

}
