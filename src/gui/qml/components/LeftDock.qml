// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Zrythm 1.0

ColumnLayout {
    ZrythmTabBar {
        id: tabBar

        Layout.fillWidth: true

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/track-inspector.svg")
            text: qsTr("Track Inspector")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/plug.svg")
            text: qsTr("Plugin Inspector")
        }

    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabBar.currentIndex

        ZrythmResizablePanel {
            title: "Browser"
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
            model: 1

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
