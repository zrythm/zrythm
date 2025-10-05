// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

import Qt.labs.synchronizer

ColumnLayout {
  id: root

  required property Project project
  readonly property int rulerHeight: 24
  readonly property int tempoMapLaneHeight: 24
  readonly property int tempoMapLaneSpacing: 1

  spacing: 0

  StackLayout {
    Layout.fillHeight: true
    Layout.fillWidth: true
    currentIndex: centerTabBar.currentIndex

    SplitView {
      Layout.fillHeight: true
      Layout.fillWidth: true
      orientation: Qt.Horizontal

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
          trackCreator: root.project.trackCreator
          tracklist: root.project.tracklist
        }

        Loader {
          id: tempoMapLegendLoader

          Layout.fillWidth: true
          Layout.preferredHeight: active ? item.implicitHeight : 0
          active: tracklistHeader.tempoMapVisible
          clip: true

          Behavior on Layout.preferredHeight {
            animation: Style.propertyAnimation
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
          pinned: true
          trackSelectionManager: root.project.trackSelectionManager
          tracklist: root.project.tracklist
          undoStack: root.project.undoStack
        }

        TracklistView {
          id: unpinnedTracklist

          Layout.fillHeight: true
          Layout.fillWidth: true
          footerPositioning: ListView.PullBackFooter
          pinned: false
          trackSelectionManager: root.project.trackSelectionManager
          tracklist: root.project.tracklist
          undoStack: root.project.undoStack

          footer: TracklistDropArea {
            id: tracklistDropArea

            width: unpinnedTracklist.width

            onFilesDropped: filePaths => {
              root.project.fileImporter.importFiles(filePaths, 0, null);
            }
          }

          Synchronizer {
            sourceObject: root.project.timeline.editorSettings
            sourceProperty: "y"
            targetObject: unpinnedTracklist
            targetProperty: "contentY"
          }
        }
      }

      // Clip Launcher View (toggleable)
      Loader {
        id: clipLauncherLoader

        SplitView.preferredWidth: tracklistHeader.clipLauncherVisible ? 400 : 0
        active: tracklistHeader.clipLauncherVisible
        visible: active

        sourceComponent: ClipLauncherView {
          clipLauncher: root.project.clipLauncher
          clipPlaybackService: root.project.clipPlaybackService
          objectCreator: root.project.arrangerObjectCreator
          rulerHeight: root.rulerHeight
          timeline: root.project.timeline
          trackCollection: root.project.tracklist.collection
        }
      }

      // Timeline View (toggleable)
      Loader {
        id: timelineLoader

        SplitView.fillHeight: true
        SplitView.fillWidth: true
        SplitView.minimumWidth: 200
        active: tracklistHeader.timelineVisible
        visible: active

        sourceComponent: ColumnLayout {
          id: timelinePane

          spacing: 1

          ScrollView {
            id: rulerScrollView

            Layout.fillWidth: true
            Layout.preferredHeight: ruler.height
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            Ruler {
              id: ruler

              editorSettings: root.project.timeline.editorSettings
              height: root.rulerHeight
              tempoMap: root.project.tempoMap
              transport: root.project.transport
            }

            Synchronizer {
              sourceObject: root.project.timeline.editorSettings
              sourceProperty: "x"
              targetObject: rulerScrollView.contentItem
              targetProperty: "contentX"
            }
          }

          Loader {
            id: tempoMapArrangerLoader

            Layout.fillWidth: true
            Layout.preferredHeight: active ? tempoMapLegendLoader.height : 0
            active: tracklistHeader.tempoMapVisible
            visible: active

            Behavior on Layout.preferredHeight {
              animation: Style.propertyAnimation
            }
            sourceComponent: TempoMapArranger {
              id: tempoMapArranger

              anchors.fill: parent
              clipEditor: root.project.clipEditor
              editorSettings: root.project.timeline.editorSettings
              laneHeight: root.tempoMapLaneHeight
              laneSpacing: root.tempoMapLaneSpacing
              objectCreator: root.project.arrangerObjectCreator
              ruler: ruler
              tempoMap: root.project.tempoMap
              tool: root.project.tool
              transport: root.project.transport
            }
          }

          Timeline {
            id: pinnedTimelineArranger

            Layout.fillWidth: true
            Layout.maximumHeight: pinnedTracklist.height
            Layout.minimumHeight: pinnedTracklist.height
            arrangerSelectionModel: arrangerSelectionModel
            clipEditor: root.project.clipEditor
            objectCreator: root.project.arrangerObjectCreator
            pinned: true
            ruler: ruler
            selectionOperator: selectionOperator
            snapGrid: root.project.snapGridTimeline
            tempoMap: root.project.tempoMap
            timeline: root.project.timeline
            tool: root.project.tool
            tracklist: root.project.tracklist
            transport: root.project.transport
            undoStack: root.project.undoStack
            unifiedObjectsModel: unifiedObjectsModel
          }

          UnifiedArrangerObjectsModel {
            id: unifiedObjectsModel

          }

          ItemSelectionModel {
            id: arrangerSelectionModel

            function getObjectFromUnifiedIndex(unifiedIndex: var): ArrangerObject {
              const sourceIndex = unifiedObjectsModel.mapToSource(unifiedIndex);
              return sourceIndex.data(ArrangerObjectListModel.ArrangerObjectPtrRole);
            }

            model: unifiedObjectsModel

            onCurrentChanged: (current, previous) => {
              if (current) {
                const arrangerObject = getObjectFromUnifiedIndex(current);
                if (ArrangerObjectHelpers.isRegion(arrangerObject)) {
                  console.log("current region changed, setting clip editor region to ", arrangerObject.name.name);
                  root.project.clipEditor.setRegion(arrangerObject, root.project.tracklist.getTrackForTimelineObject(arrangerObject));
                }
              }
            }
            onSelectionChanged: (selected, deselected) => {
              console.log("Selection changed:", selectedIndexes.length, "items selected");
              if (selectedIndexes.length > 0) {
                const firstObject = selectedIndexes[0].data(ArrangerObjectListModel.ArrangerObjectPtrRole) as ArrangerObject;
                console.log("first selected object:", firstObject);
              }

              if (deselected.length > 0) {
                deselected.forEach(deselectedRange => {
                  const arrangerObject = getObjectFromUnifiedIndex(deselectedRange.topLeft);
                  if (ArrangerObjectHelpers.isRegion(arrangerObject)) {
                    console.log("previous region changed, unsetting clip editor region");
                    root.project.clipEditor.unsetRegion();
                  }
                });
              }
            }
          }

          ArrangerObjectSelectionOperator {
            id: selectionOperator

            selectionModel: arrangerSelectionModel
            undoStack: root.project.undoStack
          }

          Timeline {
            id: unpinnedTimelineArranger

            Layout.fillHeight: true
            Layout.fillWidth: true
            arrangerSelectionModel: arrangerSelectionModel
            clipEditor: root.project.clipEditor
            objectCreator: root.project.arrangerObjectCreator
            pinned: false
            ruler: ruler
            selectionOperator: selectionOperator
            snapGrid: root.project.snapGridTimeline
            tempoMap: root.project.tempoMap
            timeline: root.project.timeline
            tool: root.project.tool
            tracklist: root.project.tracklist
            transport: root.project.transport
            undoStack: root.project.undoStack
            unifiedObjectsModel: unifiedObjectsModel
          }
        }
      }
    }

    Item {
      id: portConnectionsTab

      Layout.fillHeight: true
      Layout.fillWidth: true
    }

    Item {
      id: midiCcBindingsTab

      Layout.fillHeight: true
      Layout.fillWidth: true
    }
  }

  TabBar {
    id: centerTabBar

    Layout.fillWidth: true

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "roadmap.svg")
      text: qsTr("Arrangement")
    }

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "connector.svg")
      text: qsTr("Port Connections")
    }

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "signal-midi.svg")
      text: qsTr("Midi CC Bindings")
    }
  }
}
