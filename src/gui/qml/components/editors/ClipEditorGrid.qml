// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0

GridLayout {
  id: root

  required property var clipEditor
  required property var project
  readonly property var region: clipEditor.region
  readonly property var track: clipEditor.track

  columns: 3
  rows: 2

  RowLayout {
    id: selectedRegionInfo

    Layout.fillHeight: true
    Layout.rowSpan: 2

    Rectangle {
      Layout.fillHeight: true
      color: root.region.regionMixin.color.useColor ? root.region.regionMixin.color.color : root.track.color
      Layout.preferredWidth: 5
    }

    RotatedLabel {
      id: rotatedText

      Layout.fillHeight: true
      font: Style.normalTextFont
      text: root.region.regionMixin.name.name
    }
  }

  ZrythmToolBar {
    id: editorToolbar

    Layout.columnSpan: 2
    Layout.fillWidth: true

    centerItems: [
      ToolButton {
        text: "center"
      }
    ]
    leftItems: [
      ToolButton {
        text: "left"
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
        project: root.project
        region: root.region
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
