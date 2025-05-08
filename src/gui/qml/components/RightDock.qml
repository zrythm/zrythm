// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
    TabBar {
        id: tabBar

        Layout.fillWidth: true

        TabButton {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "shapes-large-symbolic.svg")

            ToolTip {
                text: qsTr("Plugin Browser")
            }

        }

        TabButton {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "file-cabinet-symbolic.svg")

            ToolTip {
                text: qsTr("File Browser")
            }

        }

        TabButton {
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "speaker.svg")

            ToolTip {
                text: qsTr("Monitor Section")
            }

        }

        TabButton {
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "minuet-chords.svg")

            ToolTip {
                text: qsTr("Chord Preset Browser")
            }

        }

    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabBar.currentIndex

        PluginBrowserPage {
          id: pluginBrowserPage
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
