// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

GridLayout {
  id: root

  required property AutomationEditor automationEditor
  required property ClipEditor clipEditor
  readonly property Project project: session.project
  required property ProjectSession session
  required property AutomationRegion region
  readonly property ArrangerObjectSelectionOperator selectionOperator: root.session.createArrangerObjectSelectionOperator(arrangerSelectionModel)

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
    editorSettings: root.automationEditor.editorSettings
    region: root.region
    tempoMap: root.project.tempoMap
    track: root.project.tracklist.getTrackForTimelineObject(root.region)
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
    Layout.preferredHeight: 480
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

    onSelectionChanged: (selected, deselected) => {
      console.log("Selection changed:", selectedIndexes.length, "items selected");
      if (selectedIndexes.length > 0) {
        const firstObject = selectedIndexes[0].data(ArrangerObjectListModel.ArrangerObjectPtrRole) as ArrangerObject;
        console.log("first selected object:", firstObject);
      }
    }
  }

  AutomationArranger {
    id: automationArranger

    Layout.fillHeight: true
    Layout.fillWidth: true
    arrangerContentHeight: automationLegend.height
    arrangerSelectionModel: arrangerSelectionModel
    automationEditor: root.automationEditor
    clipEditor: root.clipEditor
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
