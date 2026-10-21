// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import ZrythmArrangement
import ZrythmGui

ColumnLayout {
  id: root

  required property AudioClip audioClip
  readonly property AudioClipEditor audioEditor: clipEditor.audioEditor
  required property ClipEditor clipEditor
  readonly property Project project: session.project
  required property ProjectSession session
  readonly property Track track: project.tracklist.getTrackForTimelineObject(root.audioClip)
  /** Index of the transform entry selected in the stack panel, or -1. */
  property int selectedTransformEntryIndex: -1
  // Audio clips have no selectable child objects, so the selection model
  // stays empty
  readonly property alias selectionModel: emptySelectionModel

  spacing: 0

  onAudioClipChanged: {
    root.selectedTransformEntryIndex = -1;
  }

  ZrythmToolBar {
    id: editorToolbar

    Layout.fillWidth: true
  }

  // The ruler and the arranger share a column so they keep the same
  // width and stay aligned. Only vertical spans are used; horizontal
  // spans would feed the spanning item's width into the column-width
  // distribution and inflate the panel column.
  GridLayout {
    id: editorBody

    Layout.fillHeight: true
    Layout.fillWidth: true
    columnSpacing: 0
    columns: 2
    rowSpacing: 0
    rows: 2

    Ruler {
      id: ruler

      Layout.row: 0
      Layout.column: 1
      Layout.fillWidth: true
      clipObject: root.audioClip
      clipOperator: root.session.clipOperator
      editorSettings: root.audioEditor
      snapGrid: root.session.uiState.snapGridEditor
      tempoMap: root.project.tempoMap
      track: root.track
      transport: root.project.transport
    }

    ColumnLayout {
      Layout.row: 0
      Layout.column: 0
      Layout.rowSpan: 2
      // Nested layouts default to an expansive width policy, which would
      // let this column grow past the panel's preferred width.
      Layout.fillWidth: false
      Layout.fillHeight: true
      Layout.alignment: Qt.AlignTop

      AudioTransformStackPanel {
        id: transformStackPanel

        Layout.preferredWidth: 240
        Layout.fillHeight: true

        audioClip: root.audioClip
        selectionOperator: root.session.arrangerObjectSelectionOperator
        selectedTransformEntryIndex: root.selectedTransformEntryIndex

        onSelectionChanged: index => {
          root.selectedTransformEntryIndex = index;
        }
      }
    }

    UnifiedProxyModel {
      id: emptyModel
    }

    ItemSelectionModel {
      id: emptySelectionModel

      model: emptyModel
    }

    QObjectPropertyOperator {
      id: fadePropertyOperator

      undoStack: root.session.undoStack
    }

    AudioArranger {
      id: audioArranger

      Layout.row: 1
      Layout.column: 1
      Layout.fillHeight: true
      Layout.fillWidth: true
      arrangerSelectionModel: emptySelectionModel
      audioEditor: root.audioEditor
      clipContext: root.clipEditor.clipObject
      clipEditor: root.clipEditor
      selectedTransformEntryIndex: root.selectedTransformEntryIndex
      fadePropertyOperator: fadePropertyOperator
      objectCreator: root.session.arrangerObjectCreator
      ruler: ruler
      selectionOperator: root.session.arrangerObjectSelectionOperator
      snapGrid: root.session.uiState.snapGridEditor
      tempoMap: root.project.tempoMap
      tool: root.session.uiState.tool
      transport: root.project.transport
      undoStack: root.session.undoStack
      unifiedObjectsModel: emptyModel
    }
  }
}
