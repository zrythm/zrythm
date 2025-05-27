// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
    id: root

    required property var pluginManager

    Item {
        id: filters
    }

    Item {
        id: searchBar
    }

    ListView {
        id: pluginListView

        readonly property var
        selectionModel: ItemSelectionModel {
            model: pluginListView.model
        }

        model: root.pluginManager.pluginDescriptors
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        focus: true
        activeFocusOnTab: true
        Keys.onUpPressed: decrementCurrentIndex()
        Keys.onDownPressed: incrementCurrentIndex()

        Binding {
            target: pluginInfoLabel
            property: "descriptor"
            value: pluginListView.currentItem ? pluginListView.currentItem.descriptor : null
        }

        delegate: ItemDelegate {
            id: itemDelegate

            required property var descriptor
            required property int index

            text: descriptor.name
            width: pluginListView.width
            highlighted: ListView.isCurrentItem

            // This avoids the delegate onClicked triggering the action
            MouseArea {
                anchors.fill: parent
                onClicked: pluginListView.currentIndex = itemDelegate.index
                onDoubleClicked: itemDelegate.animateClick()
            }

            action: Action {
                text: "Activate"
                onTriggered: {
                    console.log("activated", descriptor, descriptor.name);
                    root.pluginManager.createPluginInstance(descriptor);
                }
            }

        }

        ScrollBar.vertical: ScrollBar {
        }

    }

    Label {
        id: pluginInfoLabel

        property var descriptor

        text: descriptor ? "Plugin Info:\n" + descriptor.name + "\n" + descriptor.format : "No plugin selected"
        Layout.fillWidth: true
    }

}
