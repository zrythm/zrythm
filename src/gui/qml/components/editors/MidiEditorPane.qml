// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.synchronizer
import ZrythmGui
import ZrythmStyle

GridLayout {
  id: root

  // Resolved at the playhead and updated imperatively so they stay current when
  // the chord track changes structurally or via in-place descriptor edits while
  // the playhead is stationary (a readonly binding would miss those cases).
  property ChordObject activeChord: null
  property ScaleObject activeScale: null
  readonly property ChordTrack chordTrack: root.project.tracklist.singletonTracks.chordTrack ?? null
  required property ClipEditor clipEditor
  readonly property int highlightMode: GlobalState.application.appSettings.pianoRollHighlight
  required property MidiClip midiClip
  required property MidiEditor midiEditor
  readonly property Project project: session.project
  readonly property ArrangerObjectSelectionOperator selectionOperator: root.session.createArrangerObjectSelectionOperator(arrangerSelectionModel)
  required property ProjectSession session
  readonly property Track track: root.project.tracklist.getTrackForTimelineObject(root.midiClip)

  function _updateActiveChordAndScale() {
    if (root.chordTrack) {
      const ticks = root.project.transport.playhead.ticks;
      root.activeChord = root.chordTrack.chordAtTimelineTicks(ticks);
      root.activeScale = root.chordTrack.scaleAtTimelineTicks(ticks);
    } else {
      root.activeChord = null;
      root.activeScale = null;
    }
  }

  columnSpacing: 0
  columns: 3
  rowSpacing: 0
  rows: 3

  Component.onCompleted: root._updateActiveChordAndScale()
  onChordTrackChanged: root._updateActiveChordAndScale()

  Connections {
    function onTicksChanged() {
      root._updateActiveChordAndScale();
    }

    target: root.project.transport.playhead
  }

  Connections {
    function onContentChanged() {
      root._updateActiveChordAndScale();
    }

    target: root.chordTrack?.chordClips ?? null
  }

  Connections {
    function onContentChanged() {
      root._updateActiveChordAndScale();
    }

    target: root.chordTrack?.scaleObjects ?? null
  }

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
    clipObject: root.midiClip
    clipOperator: root.session.clipOperator
    editorSettings: root.midiEditor
    snapGrid: root.session.uiState.snapGridEditor
    tempoMap: root.project.tempoMap
    track: root.project.tracklist.getTrackForTimelineObject(root.midiClip)
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

  MidiActivityProvider {
    id: noteActivityProvider

    port: root.track ? root.track.trackProcessorMidiOut : null
    portObservationManager: root.project.portObservationManager
  }

  ScrollView {
    id: pianoRollKeysScrollView

    Layout.fillHeight: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
    implicitHeight: midiArranger.implicitHeight

    PianoRollKeys {
      id: pianoRollKeys

      activeChord: root.activeChord
      activeScale: root.activeScale
      chordTrack: root.chordTrack
      height: implicitHeight
      highlightMode: root.highlightMode
      midiEditor: root.midiEditor
      noteActivityProvider: noteActivityProvider
    }

    Synchronizer {
      sourceObject: root.midiEditor
      sourceProperty: "y"
      targetObject: pianoRollKeysScrollView.contentItem
      targetProperty: "contentY"
    }
  }

  UnifiedProxyModel {
    id: unifiedObjectsModel
  }

  // Shared between MidiArranger and VelocityArranger so velocity bars react
  // to note drags (and vice versa) in real time.
  ArrangerDragState {
    id: editorDragState
  }

  ItemSelectionModel {
    id: arrangerSelectionModel

    function getObjectFromUnifiedIndex(unifiedIndex: var): ArrangerObject {
      const sourceIndex = unifiedObjectsModel.mapToSource(unifiedIndex);
      return sourceIndex.data(ArrangerObjectListModel.ArrangerObjectPtrRole);
    }

    model: unifiedObjectsModel
  }

  MidiArranger {
    id: midiArranger

    Layout.fillHeight: true
    Layout.fillWidth: true
    arrangerContentHeight: pianoRollKeys.height
    arrangerSelectionModel: arrangerSelectionModel
    chordTrack: root.chordTrack
    clipContext: root.clipEditor.clipObject
    clipEditor: root.clipEditor
    dragState: editorDragState
    highlightMode: root.highlightMode
    midiEditor: root.midiEditor
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
    dragState: editorDragState
    midiEditor: root.midiEditor
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
