// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle
import ZrythmGui

GridLayout {
  id: root

  readonly property AudioClipEditor audioEditor: clipEditor.audioEditor
  required property ClipEditor clipEditor
  readonly property Project project: session.project
  required property AudioRegion region
  required property ProjectSession session
  readonly property Track track: project.tracklist.getTrackForTimelineObject(root.region)

  columnSpacing: 0
  columns: 3
  rowSpacing: 0
  rows: 3

  ZrythmToolBar {
    id: editorToolbar

  }

  Ruler {
    id: ruler

    Layout.fillWidth: true
    editorSettings: root.audioEditor.editorSettings
    region: root.region
    snapGrid: root.session.uiState.snapGridEditor
    tempoMap: root.project.tempoMap
    track: root.track
    transport: root.project.transport
  }

  ColumnLayout {
    Layout.alignment: Qt.AlignTop
    Layout.rowSpan: 3

    ToolButton {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "chat-symbolic.svg")

      ToolTip {
        text: qsTr("Zoom In")
      }
    }
  }

  Item {
    id: placeholderLegend

    Layout.fillHeight: true
    Layout.preferredHeight: 480
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

    Layout.fillHeight: true
    Layout.fillWidth: true
    arrangerSelectionModel: emptySelectionModel
    audioEditor: root.audioEditor
    clipEditor: root.clipEditor
    fadePropertyOperator: fadePropertyOperator
    objectCreator: root.session.arrangerObjectCreator
    ruler: ruler
    selectionOperator: root.session.createArrangerObjectSelectionOperator(emptySelectionModel)
    snapGrid: root.session.uiState.snapGridEditor
    tempoMap: root.project.tempoMap
    tool: root.session.uiState.tool
    transport: root.project.transport
    undoStack: root.session.undoStack
    unifiedObjectsModel: emptyModel
  }
}
