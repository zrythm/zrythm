// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQml.Models
import Qt.labs.synchronizer
import Zrythm
import ZrythmStyle

SplitView {
  id: root

  required property PlaybackCacheActivityAggregator cacheActivityAggregator
  required property FileDialog importFileDialog
  readonly property Project project: session.project
  readonly property int rulerHeight: 24
  required property ProjectSession session
  readonly property int tempoMapLaneHeight: 24
  readonly property int tempoMapLaneSpacing: 1
  required property TrackSelectionModel trackSelectionModel

  readonly property TrackCollectionOperator trackCollectionOperator: root.session.trackCollectionOperator

  orientation: Qt.Horizontal

  Connections {
    function onTracksMoved(rows) {
      // Re-select moved tracks at their new rows. Their selection is lost
      // because moves are implemented as remove + reinsert, which invalidates
      // the persistent indices of the removed rows. Selections of tracks
      // that were not moved survive via QPersistentModelIndex row updates.
      for (const row of rows)
        root.trackSelectionModel.select(root.project.tracklist.collection.index(row, 0), ItemSelectionModel.Select);
    }

    target: root.project.tracklist.collection
  }

  ColumnLayout {
    id: leftPane

    SplitView.minimumWidth: 120
    SplitView.preferredWidth: 200
    spacing: 1

    TracklistHeader {
      id: tracklistHeader

      Layout.fillWidth: true
      Layout.maximumHeight: root.rulerHeight
      Layout.minimumHeight: root.rulerHeight
      importFileDialog: root.importFileDialog
      trackCreator: root.session.trackCreator
      tracklist: root.project.tracklist
    }

    Loader {
      id: tempoMapLegendLoader

      Layout.fillWidth: true
      Layout.preferredHeight: active ? (item as TempoMapLegend).implicitHeight : 0
      active: tracklistHeader.tempoMapVisible
      clip: true

      Behavior on Layout.preferredHeight {
        animation: ZrythmTheme.propertyAnimation
      }
      sourceComponent: TempoMapLegend {
        id: tempoMapLegend

        labelHeights: root.tempoMapLaneHeight
        spacing: root.tempoMapLaneSpacing
        tempoMap: root.project.tempoMap
      }
    }

    TracklistView {
      id: pinnedTracklist

      Layout.fillWidth: true
      Layout.preferredHeight: contentHeight
      audioEngine: root.project.engine
      pinned: true
      portObservationManager: root.project.portObservationManager
      trackCollectionOperator: root.trackCollectionOperator
      trackSelectionModel: root.trackSelectionModel
      tracklist: root.project.tracklist
      undoStack: root.session.undoStack
    }

    TracklistView {
      id: unpinnedTracklist

      Layout.fillHeight: true
      Layout.fillWidth: true
      audioEngine: root.project.engine
      pinned: false
      portObservationManager: root.project.portObservationManager
      trackCollectionOperator: root.trackCollectionOperator
      trackSelectionModel: root.trackSelectionModel
      tracklist: root.project.tracklist
      undoStack: root.session.undoStack

      footer: TracklistDropArea {
        id: tracklistDropArea

        height: 120
        width: unpinnedTracklist.width

        onFilesDropped: filePaths => {
          root.session.fileImporter.importFiles(filePaths, 0, null);
        }
        onPluginDescriptorDropped: descriptor => {
          root.session.pluginImporter.importPluginToNewTrack(descriptor);
        }
      }

      Synchronizer {
        sourceObject: root.session.uiState.timeline
        sourceProperty: "y"
        targetObject: unpinnedTracklist
        targetProperty: "contentY"
      }
    }
  }

  Loader {
    id: clipLauncherLoader

    SplitView.preferredWidth: tracklistHeader.clipLauncherVisible ? 400 : 0
    active: tracklistHeader.clipLauncherVisible
    visible: active

    sourceComponent: ClipLauncherView {
      clipLauncher: root.project.clipLauncher
      clipPlaybackService: root.project.clipPlaybackService
      fileImporter: root.session.fileImporter
      objectCreator: root.session.arrangerObjectCreator
      rulerHeight: root.rulerHeight
      timeline: root.session.uiState.timeline
      trackCollection: root.project.tracklist.collection
      tracklist: root.project.tracklist
    }
  }

  Loader {
    id: timelineLoader

    SplitView.fillHeight: true
    SplitView.fillWidth: true
    SplitView.minimumWidth: 200
    active: tracklistHeader.timelineVisible
    visible: active

    sourceComponent: ColumnLayout {
      id: timelinePane

      readonly property ArrangerObjectSelectionOperator selectionOperator: root.session.arrangerObjectSelectionOperator
      readonly property TimelineArrangerObjectsModel unifiedObjectsModel: root.session.timelineArrangerObjects

      spacing: 1

      Ruler {
        id: timelineRuler

        Layout.fillWidth: true
        Layout.preferredHeight: root.rulerHeight
        playbackCacheCompleteCount: root.cacheActivityAggregator.cacheCompleteCount
        playbackCachePendingCount: root.cacheActivityAggregator.cachePendingCount
        editorSettings: root.session.uiState.timeline
        snapGrid: root.session.uiState.snapGridTimeline
        tempoMap: root.project.tempoMap
        transport: root.project.transport
      }

      Loader {
        id: tempoMapArrangerLoader

        Layout.fillWidth: true
        Layout.preferredHeight: active ? tempoMapLegendLoader.height : 0
        active: tracklistHeader.tempoMapVisible
        visible: active

        Behavior on Layout.preferredHeight {
          animation: ZrythmTheme.propertyAnimation
        }
        sourceComponent: TempoMapArranger {
          id: tempoMapArranger

          anchors.fill: parent
          arrangerSelectionModel: timelineArrangerSelectionModel
          clipEditor: root.session.uiState.clipEditor
          editorSettings: root.session.uiState.timeline
          laneHeight: root.tempoMapLaneHeight
          laneSpacing: root.tempoMapLaneSpacing
          objectCreator: root.session.arrangerObjectCreator
          ruler: timelineRuler
          selectionOperator: timelinePane.selectionOperator
          snapGrid: root.session.uiState.snapGridTimeline
          tempoMap: root.project.tempoMap
          tempoObjectManager: root.project.tempoObjectManager
          tool: root.session.uiState.tool
          tracklist: root.project.tracklist
          transport: root.project.transport
          undoStack: root.session.undoStack
          unifiedObjectsModel: timelinePane.unifiedObjectsModel
        }
      }

      Timeline {
        id: pinnedTimelineArranger

        Layout.fillWidth: true
        Layout.maximumHeight: pinnedTracklist.height
        Layout.minimumHeight: pinnedTracklist.height
        arrangerSelectionModel: timelineArrangerSelectionModel
        clipEditor: root.session.uiState.clipEditor
        fileImporter: root.session.fileImporter
        objectCreator: root.session.arrangerObjectCreator
        pinned: true
        ruler: timelineRuler
        selectionOperator: timelinePane.selectionOperator
        snapGrid: root.session.uiState.snapGridTimeline
        tempoMap: root.project.tempoMap
        tempoObjectManager: root.project.tempoObjectManager
        timeline: root.session.uiState.timeline
        tool: root.session.uiState.tool
        tracklist: root.project.tracklist
        trackSelectionModel: root.trackSelectionModel
        transport: root.project.transport
        undoStack: root.session.undoStack
        unifiedObjectsModel: timelinePane.unifiedObjectsModel
      }

      ItemSelectionModel {
        id: timelineArrangerSelectionModel

        function getObjectFromUnifiedIndex(unifiedIndex: var): ArrangerObject {
          const sourceIndex = unifiedObjectsModel.mapToSource(unifiedIndex);
          return sourceIndex.data(ArrangerObjectListModel.ArrangerObjectPtrRole);
        }

        model: unifiedObjectsModel

        onCurrentChanged: (current, previous) => {
          if (current) {
            const arrangerObject = getObjectFromUnifiedIndex(current);
            if (ArrangerObjectHelper.isClip(arrangerObject)) {
              root.session.uiState.clipEditor.setClip(arrangerObject, root.project.tracklist.getTrackForTimelineObject(arrangerObject));
            }
          }
        }
        onSelectionChanged: (selected, deselected) => {
          if (deselected.length > 0) {
            deselected.forEach(deselectedRange => {
              const arrangerObject = getObjectFromUnifiedIndex(deselectedRange.topLeft);
              if (ArrangerObjectHelper.isClip(arrangerObject)) {
                root.session.uiState.clipEditor.unsetClip();
              }
            });
          }
        }
      }

      Timeline {
        id: unpinnedTimelineArranger

        Layout.fillHeight: true
        Layout.fillWidth: true
        arrangerSelectionModel: timelineArrangerSelectionModel
        clipEditor: root.session.uiState.clipEditor
        fileImporter: root.session.fileImporter
        objectCreator: root.session.arrangerObjectCreator
        pinned: false
        ruler: timelineRuler
        selectionOperator: timelinePane.selectionOperator
        snapGrid: root.session.uiState.snapGridTimeline
        tempoMap: root.project.tempoMap
        tempoObjectManager: root.project.tempoObjectManager
        timeline: root.session.uiState.timeline
        tool: root.session.uiState.tool
        tracklist: root.project.tracklist
        trackSelectionModel: root.trackSelectionModel
        transport: root.project.transport
        undoStack: root.session.undoStack
        unifiedObjectsModel: timelinePane.unifiedObjectsModel

        arrangerContentHeight: Math.max(unpinnedTracklist.contentHeight, unpinnedTimelineArranger.height)
      }

      TimelineMinimap {
        id: timelineMinimap

        Layout.fillWidth: true
        Layout.preferredHeight: 32
        editorSettings: root.session.uiState.timeline
        ruler: timelineRuler
        scrollViewWidth: unpinnedTimelineArranger.scrollViewWidth
        tracklist: root.project.tracklist
      }
    }
  }
}
