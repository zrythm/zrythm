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

        ZrythmResizablePanel {
            title: "Piano Roll"
            vertical: true

            content: Rectangle {
                color: "#1C1C1C"

                Label {
                    anchors.centerIn: parent
                    text: "Piano Roll content"
                    color: "white"
                }

            }

        }

        Repeater {
            model: 3

            Rectangle {
                width: 180
                height: 50
                color: Qt.rgba(Math.random(), Math.random(), Math.random(), 1)
                border.color: "black"
                border.width: 2

                Text {
                    anchors.centerIn: parent
                    text: "Item " + (index + 1)
                    font.pixelSize: 16
                }

            }

        }

    }

    TabBar {
        id: centerTabBar

        Layout.fillWidth: true

        TabButton {
            icon.source: Style.getIcon("zrythm-dark", "piano-roll.svg")
            text: qsTr("Editor")
        }

        TabButton {
            icon.source: Style.getIcon("zrythm-dark", "mixer.svg")
            text: qsTr("Mixer")
        }

        TabButton {
            icon.source: Style.getIcon("gnome-icon-library", "encoder-knob-symbolic.svg")
            text: qsTr("Modulators")
        }

        TabButton {
            icon.source: Style.getIcon("zrythm-dark", "chord-pad.svg")
            text: qsTr("Chord Pad")
        }

    }

}
