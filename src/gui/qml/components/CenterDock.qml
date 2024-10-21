// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
    spacing: 0

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: centerTabBar.currentIndex

        ZrythmArranger {
            id: timelineArranger

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Item {
            id: portConnectionsTab

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Item {
            id: midiCcBindingsTab

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

    }

    TabBar {
        id: centerTabBar

        Layout.fillWidth: true

        TabButton {
            icon.source: Style.getIcon("zrythm-dark", "roadmap.svg")
            text: qsTr("Timeline")
        }

        TabButton {
            icon.source: Style.getIcon("zrythm-dark", "connector.svg")
            text: qsTr("Port Connections")
        }

        TabButton {
            icon.source: Style.getIcon("zrythm-dark", "signal-midi.svg")
            text: qsTr("Midi CC Bindings")
        }

    }

}
