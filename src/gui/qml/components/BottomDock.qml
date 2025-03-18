// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
    id: root

    required property var project

    spacing: 0

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: centerTabBar.currentIndex

        StackLayout {
            id: editorContentAndNoRegionLabelStack

            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.project.clipEditor.region ? 1 : 0

            PlaceholderPage {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                icon.source: ResourceManager.getIconUrl("zrythm-dark", "roadmap.svg")
                title: qsTr("No clip selected")
                description: qsTr("Select a clip from the timeline")
            }

            Loader {
                active: editorContentAndNoRegionLabelStack.currentIndex === 1
                visible: active

                sourceComponent: ClipEditorGrid {
                    project: root.project
                    clipEditor: root.project.clipEditor
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
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "piano-roll.svg")
            text: qsTr("Editor")
        }

        TabButton {
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "mixer.svg")
            text: qsTr("Mixer")
        }

        TabButton {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "encoder-knob-symbolic.svg")
            text: qsTr("Modulators")
        }

        TabButton {
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "chord-pad.svg")
            text: qsTr("Chord Pad")
        }

    }

}
