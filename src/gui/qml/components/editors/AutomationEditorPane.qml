// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

GridLayout {
  id: root

  required property AutomationClip automationClip
  required property AutomationEditor automationEditor
  required property ClipEditor clipEditor
  readonly property Project project: session.project
  readonly property ArrangerObjectSelectionOperator selectionOperator: root.session.createArrangerObjectSelectionOperator(arrangerSelectionModel)
  required property ProjectSession session

  columnSpacing: 0
  columns: 3
  rowSpacing: 0
  rows: 3

  ZrythmToolBar {
    id: topOfPianoRollToolbar

    leftItems: [
      ToolButton {
        checkable: true
        checked: false
        icon.source: ResourceManager.getIconUrl("font-awesome", "drum-solid.svg")

        ToolTip {
          text: qsTr("Drum Notation")
        }
      }
    ]
  }

  Ruler {
    id: ruler

    Layout.fillWidth: true
    clipObject: root.automationClip
    clipOperator: root.session.clipOperator
    editorSettings: root.automationEditor
    snapGrid: root.session.uiState.snapGridEditor
    tempoMap: root.project.tempoMap
    track: root.project.tracklist.getTrackForTimelineObject(root.automationClip)
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
    id: automationLegend

    Layout.fillHeight: true
  }

  UnifiedProxyModel {
    id: unifiedObjectsModel
  }

  ItemSelectionModel {
    id: arrangerSelectionModel

    function getObjectFromUnifiedIndex(unifiedIndex: var): ArrangerObject {
      const sourceIndex = unifiedObjectsModel.mapToSource(unifiedIndex);
      return sourceIndex.data(ArrangerObjectListModel.ArrangerObjectPtrRole);
    }

    model: unifiedObjectsModel
  }

  AutomationArranger {
    id: automationArranger

    Layout.fillHeight: true
    Layout.fillWidth: true
    arrangerContentHeight: automationLegend.height
    arrangerSelectionModel: arrangerSelectionModel
    automationEditor: root.automationEditor
    clipContext: root.clipEditor.clipObject
    clipEditor: root.clipEditor
    uuidPropertyOperator: root.session.uuidPropertyOperator
    objectCreator: root.session.arrangerObjectCreator
    ruler: ruler
    selectionOperator: root.selectionOperator
    snapGrid: root.session.uiState.snapGridEditor
    tempoMap: root.project.tempoMap
    tool: root.session.uiState.tool
    transport: root.project.transport
    undoStack: root.session.undoStack
    unifiedObjectsModel: unifiedObjectsModel
  }
}
