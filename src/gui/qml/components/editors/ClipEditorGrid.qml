// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.synchronizer
import Zrythm

GridLayout {
  id: root

  required property ClipEditor clipEditor
  readonly property Project project: session.project
  readonly property var region: clipEditor.region
  required property ProjectSession session
  readonly property var track: clipEditor.track

  columns: 2
  rows: 2

  RowLayout {
    id: selectedRegionInfo

    Layout.fillHeight: true
    Layout.rowSpan: 2

    Rectangle {
      Layout.fillHeight: true
      Layout.preferredWidth: 5
      color: root.region.color.useColor ? root.region.color.color : root.track.color
    }

    RotatedLabel {
      id: rotatedText

      Layout.fillHeight: true
      font: ZrythmTheme.normalTextFont
      text: root.region.name.name
    }
  }

  ZrythmToolBar {
    id: editorToolbar

    Layout.fillWidth: true

    leftItems: [
      SnapGridButton {
        snapGrid: root.session.uiState.snapGridEditor
      },
      RowLayout {
        visible: root.region.type === ArrangerObject.MidiRegion

        Label {
          Layout.alignment: Qt.AlignVCenter
          text: qsTr("Highlight")
        }

        ComboBox {
          model: [
            {
              text: qsTr("No Highlight"),
              value: MidiEditor.None
            },
            {
              text: qsTr("Chord"),
              value: MidiEditor.Chord
            },
            {
              text: qsTr("Scale"),
              value: MidiEditor.Scale
            },
            {
              text: qsTr("Scale + Chord"),
              value: MidiEditor.Both
            }
          ]
          textRole: "text"
          valueRole: "value"

          Synchronizer on currentValue {
            sourceObject: GlobalState.application.appSettings
            sourceProperty: "pianoRollHighlight"
          }

          ToolTip {
            text: qsTr("Piano Roll Highlighting")
          }
        }
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
        return 0;
      case ArrangerObject.AudioRegion:
        return 1;
      case ArrangerObject.ChordRegion:
        return 3;
      case ArrangerObject.AutomationRegion:
        return 2;
      default:
        return 0;
      }
    }

    Loader {
      id: midiEditorLoader

      active: editorSpecializedStack.currentIndex === 0

      sourceComponent: MidiEditorPane {
        clipEditor: root.clipEditor
        midiEditor: root.clipEditor.midiEditor
        region: root.region
        session: root.session
      }
    }

    Loader {
      id: audioEditorLoader

      active: editorSpecializedStack.currentIndex === 1

      sourceComponent: AudioEditorPane {
        clipEditor: root.clipEditor
        region: root.region
        session: root.session
      }
    }

    Loader {
      id: automationEditorLoader

      active: editorSpecializedStack.currentIndex === 2

      sourceComponent: AutomationEditorPane {
        automationEditor: root.clipEditor.automationEditor
        clipEditor: root.clipEditor
        region: root.region
        session: root.session
      }
    }

    Loader {
      id: chordEditorLoader

      active: editorSpecializedStack.currentIndex === 3

      sourceComponent: ChordEditorPane {
        clipEditor: root.clipEditor
        region: root.region
        session: root.session
      }
    }
  }
}
