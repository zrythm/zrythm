// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0

GridLayout {
    id: root

    required property var project
    required property var clipEditor
    readonly property var region: clipEditor.region

    rows: 2
    columns: 3

    RowLayout {
        id: selectedRegionInfo

        Layout.fillHeight: true
        Layout.rowSpan: 2

        Rectangle {
            color: region.effectiveColor
            Layout.fillHeight: true
            width: 5
        }

        RotatedLabel {
            id: rotatedText

            font: Style.normalTextFont
            Layout.fillHeight: true
            text: root.region.name
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

        currentIndex: root.region.regionType

        Loader {
            id: midiEditorLoader

            active: editorSpecializedStack.currentIndex === 0

            sourceComponent: MidiEditorPane {
                project: root.project
                region: root.region
                clipEditor: root.clipEditor
                pianoRoll: root.clipEditor.pianoRoll
            }

        }

        Loader {
            id: audioEditorLoader

            active: editorSpecializedStack.currentIndex === 1

            sourceComponent: AudioEditorPane {
                project: root.project
                region: root.region
            }

        }

        Loader {
            id: automationEditorLoader

            active: editorSpecializedStack.currentIndex === 2

            sourceComponent: AutomationEditorPane {
                project: root.project
                region: root.region
            }

        }

        Loader {
            id: chordEditorLoader

            active: editorSpecializedStack.currentIndex === 3

            sourceComponent: ChordEditorPane {
                project: root.project
                region: root.region
            }

        }

    }

}
