// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
    TabBar {
        id: tabBar

        Layout.fillWidth: true

        TabButton {
            icon.source: Style.getIcon("gnome-icon-library", "shapes-large-symbolic.svg")

            ToolTip {
                text: qsTr("Plugin Browser")
            }

        }

        TabButton {
            icon.source: Style.getIcon("gnome-icon-library", "file-cabinet-symbolic.svg")

            ToolTip {
                text: qsTr("File Browser")
            }

        }

        TabButton {
            icon.source: Style.getIcon("zrythm-dark", "speaker.svg")

            ToolTip {
                text: qsTr("Monitor Section")
            }

        }

        TabButton {
            icon.source: Style.getIcon("zrythm-dark", "minuet-chords.svg")

            ToolTip {
                text: qsTr("Chord Preset Browser")
            }

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
