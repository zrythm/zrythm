// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
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

        GridLayout {
            id: editorContentGrid

            rows: 2
            columns: 3

            RowLayout {
                id: selectedRegionInfo

                Layout.fillHeight: true
                Layout.rowSpan: 2

                Rectangle {
                    color: "orange"
                    Layout.fillHeight: true
                    width: 5
                }

                RotatedLabel {
                    id: rotatedText

                    font: Style.normalTextFont
                    Layout.fillHeight: true
                    text: "Audio Region 1"
                }

            }

            ZrythmToolBar {
                id: editorToolbar

                Layout.columnSpan: 2
                Layout.fillWidth: true
                leftItems: [
                    ToolButton {
                        text: "left"
                    }
                ]
                centerItems: [
                    ToolButton {
                        text: "center"
                    }
                ]
                rightItems: [
                    ToolButton {
                        text: "right"
                    }
                ]
            }

            StackLayout {
                id: editorSpecializedStack
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
