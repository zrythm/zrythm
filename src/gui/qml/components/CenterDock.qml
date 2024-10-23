// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
    required property var project

    spacing: 0

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: centerTabBar.currentIndex

        GridLayout {
            rows: 2
            columns: 2
            rowSpacing: 0
            columnSpacing: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                text: "track header"
            }

            Label {
                text: "ruler"
            }

            Tracklist {
                tracklist: project.tracklist
                pinned: true
            }

            Timeline {
                id: pinnedTimelineArranger

                pinned: true
                timeline: project.timeline
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            Tracklist {
                tracklist: project.tracklist
                pinned: false
            }

            Timeline {
                id: unpinnedTimelineArranger

                pinned: false
                timeline: project.timeline
                     Layout.fillWidth: true
                Layout.fillHeight: true
            }

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
        Layout.fillHeight: true

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
