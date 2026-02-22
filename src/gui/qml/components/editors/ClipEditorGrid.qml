// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

GridLayout {
  id: root

  required property ClipEditor clipEditor
  readonly property Project project: session.project
  required property ProjectSession session
  readonly property var region: clipEditor.region
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
      font: Style.normalTextFont
      text: root.region.name.name
    }
  }

  ZrythmToolBar {
    id: editorToolbar

    Layout.fillWidth: true

    centerItems: [
      ToolButton {
        text: "center"
      }
    ]
    leftItems: [
      SnapGridButton {
        snapGrid: root.session.uiState.snapGridEditor
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
        pianoRoll: root.clipEditor.pianoRoll
        session: root.session
        region: root.region
      }
    }

    Loader {
      id: audioEditorLoader

      active: editorSpecializedStack.currentIndex === 1

      sourceComponent: AudioEditorPane {
        session: root.session
        region: root.region
      }
    }

    Loader {
      id: automationEditorLoader

      active: editorSpecializedStack.currentIndex === 2

      sourceComponent: AutomationEditorPane {
        clipEditor: root.clipEditor
        automationEditor: root.clipEditor.automationEditor
        session: root.session
        region: root.region
      }
    }

    Loader {
      id: chordEditorLoader

      active: editorSpecializedStack.currentIndex === 3

      sourceComponent: ChordEditorPane {
        session: root.session
        region: root.region
      }
    }
  }
}
