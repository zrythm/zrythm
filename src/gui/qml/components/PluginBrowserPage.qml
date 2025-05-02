// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
    id: root

    Item {
        id: filters
    }

    Item {
        id: searchBar
    }

    ListView {
        // TODO

        id: pluginListView

        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    Label {
        id: pluginInfoLabel

        text: "Plugin Info"
        Layout.fillWidth: true
    }

}
