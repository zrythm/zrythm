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
    readonly property var track: clipEditor.track

    rows: 2
    columns: 3

    RowLayout {
        id: selectedRegionInfo

        Layout.fillHeight: true
        Layout.rowSpan: 2

        Rectangle {
            color: region.regionMixin.color.useColor ? region.regionMixin.color.color : track.color
            Layout.fillHeight: true
            width: 5
        }

        RotatedLabel {
            id: rotatedText

            font: Style.normalTextFont
            Layout.fillHeight: true
            text: root.region.regionMixin.name.name
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

        currentIndex: {
          switch (root.region.type) {
            case ArrangerObject.MidiRegion:
              return 0
            case ArrangerObject.AudioRegion:
              return 1
            case ArrangerObject.ChordRegion:
              return 3
            case ArrangerObject.AutomationRegion:
              return 2
            default:
              return 0
          }
        }

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
