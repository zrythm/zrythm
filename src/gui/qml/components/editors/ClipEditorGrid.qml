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
  readonly property var clipObject: clipEditor.clipObject
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
      color: root.clipObject.color.useColor ? root.clipObject.color.color : root.track.color
    }

    RotatedLabel {
      id: rotatedText

      Layout.fillHeight: true
      font: ZrythmTheme.normalTextFont
      text: root.clipObject.name.name
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
        visible: root.clipObject.type === ArrangerObject.MidiClip

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
      switch (root.clipObject.type) {
      case ArrangerObject.MidiClip:
        return 0;
      case ArrangerObject.AudioClip:
        return 1;
      case ArrangerObject.ChordClip:
        return 3;
      case ArrangerObject.AutomationClip:
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
        midiClip: root.clipObject as MidiClip
        midiEditor: root.clipEditor.midiEditor
        session: root.session
      }
    }

    Loader {
      id: audioEditorLoader

      active: editorSpecializedStack.currentIndex === 1

      sourceComponent: AudioEditorPane {
        audioClip: root.clipObject as AudioClip
        clipEditor: root.clipEditor
        session: root.session
      }
    }

    Loader {
      id: automationEditorLoader

      active: editorSpecializedStack.currentIndex === 2

      sourceComponent: AutomationEditorPane {
        automationClip: root.clipObject as AutomationClip
        automationEditor: root.clipEditor.automationEditor
        clipEditor: root.clipEditor
        session: root.session
      }
    }

    Loader {
      id: chordEditorLoader

      active: editorSpecializedStack.currentIndex === 3

      sourceComponent: ChordEditorPane {
        chordClip: root.clipObject as ChordClip
        clipEditor: root.clipEditor
        session: root.session
      }
    }
  }
}
