// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Zrythm 1.0

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

    ZrythmTabBar {
        id: centerTabBar

        Layout.fillWidth: true

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/piano-roll.svg")
            text: qsTr("Editor")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/mixer.svg")
            text: qsTr("Mixer")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/gnome-icon-library/encoder-knob-symbolic.svg")
            text: qsTr("Modulators")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/chord-pad.svg")
            text: qsTr("Chord Pad")
        }

    }

}
