// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  required property PluginImporter pluginImporter
  required property PluginManager pluginManager

  signal pluginDescriptorActivated(PluginDescriptor descriptor)

  onPluginDescriptorActivated: descriptor => pluginImporter.importPlugin(descriptor)

  Item {
    id: filters

  }

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
    delegate: ItemDelegate {
      id: itemDelegate

      required property PluginDescriptor descriptor
      required property int index

      highlighted: ListView.isCurrentItem
      text: descriptor?.name
      width: pluginListView.width

      action: Action {
        text: "Activate"

        onTriggered: {
          console.log("activated", itemDelegate.descriptor, itemDelegate.descriptor.name);
          root.pluginDescriptorActivated(itemDelegate.descriptor);
        }
      }

      // This avoids the delegate onClicked triggering the action
      MouseArea {
        anchors.fill: parent

        onClicked: {
          pluginListView.currentIndex = itemDelegate.index;
        }
        onDoubleClicked: itemDelegate.animateClick()
      }
    }

    Keys.onDownPressed: incrementCurrentIndex()
    Keys.onUpPressed: decrementCurrentIndex()

    Binding {
      property: "descriptor"
      target: pluginInfoLabel
      value: pluginListView.currentItem ? pluginListView.currentItem.descriptor : null
    }
  }

  Label {
    id: pluginInfoLabel

    property var descriptor

    Layout.fillWidth: true
    text: descriptor ? "Plugin Info:\n" + descriptor.name + "\n" + descriptor.format : "No plugin selected"
  }

  component RoleData: QtObject {
    property string name
  }
}
