// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
//import QtQuick.Controls.Basic
import ZrythmStyle 1.0
import QtQuick.Layouts
import Zrythm 1.0

ColumnLayout {
    TabBar {
        id: tabBar

        Layout.fillWidth: true

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/gnome-icon-library/puzzle-piece-symbolic.svg")
            text: qsTr("Plugin Browser")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/gnome-icon-library/library-music-symbolic.svg")
            text: qsTr("File Browser")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/gnome-icon-library/speakers-symbolic.svg")
            text: qsTr("Monitor Section")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/minuet-chords.svg")
            text: qsTr("Chord Preset Browser")
        }

    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabBar.currentIndex

        ZrythmResizablePanel {
            title: "Other"
            vertical: false

            content: Rectangle {
                color: "#2C2C2C"

                Label {
                    anchors.centerIn: parent
                    text: "Browser content"
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

}
