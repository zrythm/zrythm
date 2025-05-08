// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

GridLayout {
    id: root

    required property var project
    required property var clipEditor
    required property var pianoRoll
    required property var region

    rows: 3
    columns: 3
    columnSpacing: 0
    rowSpacing: 0

    ZrythmToolBar {
        id: topOfPianoRollToolbar

        leftItems: [
            ToolButton {
                icon.source: ResourceManager.getIconUrl("font-awesome", "drum-solid.svg")
                checkable: true
                checked: false

                ToolTip {
                    text: qsTr("Drum Notation")
                }

            },
            ToolButton {
                icon.source: ResourceManager.getIconUrl("zrythm-dark", "audio-volume-high.svg")
                checkable: true
                checked: true

                ToolTip {
                    text: qsTr("Listen Notes")
                }

            },
            ToolButton {
                icon.source: ResourceManager.getIconUrl("gnome-icon-library", "chat-symbolic.svg")
                checkable: true
                checked: false

                ToolTip {
                    text: qsTr("Show Automation Values")
                }

            }
        ]
    }

    Ruler {
        id: ruler

        Layout.fillWidth: true
        editorSettings: project.clipEditor.pianoRoll
        transport: project.transport
    }

    ColumnLayout {
        Layout.rowSpan: 3
        Layout.alignment: Qt.AlignTop

        ToolButton {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "chat-symbolic.svg")

            ToolTip {
                text: qsTr("Zoom In")
            }

        }

    }

    ScrollView {
        id: pianoRollKeysScrollView

        Layout.fillHeight: true
        Layout.preferredHeight: 480
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff

        PianoRollKeys {
            id: pianoRollKeys

            pianoRoll: root.pianoRoll
        }

        Binding {
            target: pianoRollKeysScrollView.contentItem
            property: "contentY"
            value: root.pianoRoll.y
        }

        Connections {
            function onContentYChanged() {
                root.pianoRoll.y = pianoRollKeysScrollView.contentItem.contentY;
            }

            target: pianoRollKeysScrollView.contentItem
        }

    }

    MidiArranger {
        id: midiArranger

        Layout.fillWidth: true
        Layout.fillHeight: true
        clipEditor: root.clipEditor
        pianoRoll: root.pianoRoll
        objectFactory: root.project.arrangerObjectFactory
        ruler: ruler
        tool: project.tool
    }

    Rectangle {
        Label {
            text: qsTr("Velocity")
        }

    }

    VelocityArranger {
        id: velocityArranger

        Layout.fillWidth: true
        Layout.fillHeight: true
        clipEditor: root.clipEditor
        pianoRoll: root.pianoRoll
        objectFactory: root.project.arrangerObjectFactory
        ruler: ruler
        tool: project.tool
    }

}
