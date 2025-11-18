// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

Item {
  id: root

  required property PluginManager pluginManager

  signal pluginDescriptorActivated(PluginDescriptor descriptor)

  SortFilterProxyModel {
    id: pluginFilter

    model: root.pluginManager.pluginDescriptors

    filters: [
      FunctionFilter {
        property var regExp: new RegExp(searchText, "i")
        property string searchText: pluginSearch.text

        function filter(data: RoleData): bool {
          return regExp.test(data.name);
        }

        onRegExpChanged: invalidate()
      }
    ]
    sorters: [
      RoleSorter {
        priority: 0
        roleName: "name"
      }
    ]
  }

  Image {
    id: draggable

    property PluginDescriptor descriptor

    Drag.dragType: Drag.Automatic
    visible: Drag.active

    Drag.onDragFinished: {}
    Drag.onDragStarted: {}
  }

  ColumnLayout {
    anchors.fill: parent

    SearchField {
      id: pluginSearch

      Layout.fillWidth: true
    }

    ListView {
      id: pluginListView

      readonly property var selectionModel: ItemSelectionModel {
        model: pluginListView.model
      }

      Layout.fillHeight: true
      Layout.fillWidth: true
      activeFocusOnTab: true
      boundsBehavior: Flickable.StopAtBounds
      clip: true
      focus: true
      model: pluginFilter

      ScrollBar.vertical: ScrollBar {
      }
      delegate: PluginDescriptorRow {
        width: pluginListView.width
      }

      Keys.onDownPressed: incrementCurrentIndex()
      Keys.onUpPressed: decrementCurrentIndex()

      Binding {
        property: "descriptor"
        target: pluginInfoLabel
        value: pluginListView.currentItem ? (pluginListView.currentItem as PluginDescriptorRow).descriptor : null
      }
    }

    Label {
      id: pluginInfoLabel

      property var descriptor

      Layout.fillWidth: true
      text: descriptor ? "Plugin Info:\n" + descriptor.name + "\n" + descriptor.format : "No plugin selected"
    }
  }

  component PluginDescriptorRow: ItemDelegate {
    id: pluginDescriptorItemDelegate

    required property PluginDescriptor descriptor
    required property int index

    highlighted: ListView.isCurrentItem
    icon.height: 16
    icon.source: {
      if (descriptor.isInstrument()) {
        return ResourceManager.getIconUrl("zrythm-dark", "instrument.svg");
      } else if (descriptor.isMidiModifier()) {
        return ResourceManager.getIconUrl("zrythm-dark", "signal-midi.svg");
      } else {
        return ResourceManager.getIconUrl("zrythm-dark", "audio-insert.svg");
      }
    }
    icon.width: 16
    text: descriptor?.name

    DragHandler {
      id: dragHandler

      target: draggable

      onActiveChanged: {
        if (active) {
          pluginListView.currentIndex = pluginDescriptorItemDelegate.index;
          target.width = parent.width;
          target.height = parent.height;
          const size = Qt.size(parent.width, parent.height);
          parent.grabToImage(function (result) {
            target.source = result.url;
            target.Drag.mimeData = {
              "application/x-plugin-descriptor": pluginDescriptorItemDelegate.descriptor.serializeToString()
            };
            target.descriptor = parent.descriptor;
            target.Drag.active = true;
          }, size);
        } else {
          target.Drag.active = false;
        }
      }
    }

    TapHandler {
      onDoubleTapped: root.pluginDescriptorActivated(pluginDescriptorItemDelegate.descriptor)
      onTapped: pluginListView.currentIndex = pluginDescriptorItemDelegate.index
    }
  }
  component RoleData: QtObject {
    property string name
  }
}
