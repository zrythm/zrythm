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

        model: root.pluginManager.pluginDescriptors
        delegate: ItemDelegate {
          required property var descriptor

          Label {
            text: descriptor.name
          }
        }

        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    Label {
        id: pluginInfoLabel

        text: "Plugin Info"
        Layout.fillWidth: true
    }

}
