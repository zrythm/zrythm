// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0

GridLayout {
    id: root

    required property var region

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

        currentIndex: 0

        MidiEditorPane {
            id: midiEditorPane
        }

        AudioEditorPane {
            id: audioEditorPane
        }

        AutomationEditorPane {
            id: automationEditorPane
        }

        ChordEditorPane {
            id: chordEditorPane
        }

    }

}
