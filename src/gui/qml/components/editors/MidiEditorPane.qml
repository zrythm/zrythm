// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmGui
import ZrythmStyle

import Qt.labs.synchronizer

GridLayout {
  id: root

  required property ClipEditor clipEditor
  required property PianoRoll pianoRoll
  readonly property Project project: session.project
  required property ProjectSession session
  required property MidiRegion region
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
      },
      ToolButton {
        checkable: true
        checked: true
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "audio-volume-high.svg")

        ToolTip {
          text: qsTr("Listen Notes")
        }
      },
      ToolButton {
        checkable: true
        checked: false
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "chat-symbolic.svg")

        ToolTip {
          text: qsTr("Show Automation Values")
        }
      }
    ]
  }

  Ruler {
    id: ruler

    Layout.fillWidth: true
    editorSettings: root.pianoRoll.editorSettings
    tempoMap: root.project.tempoMap
    transport: root.project.transport
    region: root.region
    track: root.project.tracklist.getTrackForTimelineObject(root.region)
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

  ScrollView {
    id: pianoRollKeysScrollView

    Layout.fillHeight: true
    Layout.preferredHeight: 480
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AlwaysOff

    PianoRollKeys {
      id: pianoRollKeys

      height: implicitHeight
      pianoRoll: root.pianoRoll
    }

    Synchronizer {
      sourceObject: root.pianoRoll.editorSettings
      sourceProperty: "y"
      targetObject: pianoRollKeysScrollView.contentItem
      targetProperty: "contentY"
    }
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

  MidiArranger {
    id: midiArranger

    Layout.fillHeight: true
    Layout.fillWidth: true
    arrangerContentHeight: pianoRollKeys.height
    arrangerSelectionModel: arrangerSelectionModel
    clipEditor: root.clipEditor
    objectCreator: root.session.arrangerObjectCreator
    pianoRoll: root.pianoRoll
    ruler: ruler
    selectionOperator: root.selectionOperator
    snapGrid: root.session.uiState.snapGridEditor
    tempoMap: root.project.tempoMap
    tool: root.session.uiState.tool
    transport: root.project.transport
    undoStack: root.session.undoStack
    unifiedObjectsModel: unifiedObjectsModel
  }

  Rectangle {
    Label {
      text: qsTr("Velocity")
    }
  }

  VelocityArranger {
    id: velocityArranger

    Layout.fillHeight: true
    Layout.fillWidth: true
    arrangerSelectionModel: arrangerSelectionModel
    clipEditor: root.clipEditor
    objectCreator: root.session.arrangerObjectCreator
    pianoRoll: root.pianoRoll
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
