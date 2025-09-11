// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  required property Project project
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
          Layout.maximumHeight: ruler.height
          Layout.minimumHeight: ruler.height
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

          Layout.fillWidth: true
          Layout.maximumHeight: unpinnedTimelineArranger.height
          Layout.minimumHeight: unpinnedTimelineArranger.height
          footerPositioning: ListView.PullBackFooter
          pinned: false
          trackSelectionManager: root.project.trackSelectionManager
          tracklist: root.project.tracklist
          undoStack: root.project.undoStack

          footer: TracklistDropArea {
            id: tracklistDropArea

            width: unpinnedTracklist.width

            onFilesDropped: filePaths => {
              root.project.trackCreator.importFiles(filePaths, 0, null);
            }
          }
        }
      }

      ColumnLayout {
        id: rightPane

        SplitView.fillHeight: true
        SplitView.fillWidth: true
        SplitView.minimumWidth: 200
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
            tempoMap: root.project.tempoMap
            transport: root.project.transport
          }

          Binding {
            property: "contentX"
            target: rulerScrollView.contentItem
            value: root.project.timeline.editorSettings.x
          }

          Connections {
            function onContentXChanged() {
              root.project.timeline.editorSettings.x = rulerScrollView.contentItem.contentX;
            }

            target: rulerScrollView.contentItem
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
              if (arrangerObject && arrangerObject.regionMixin) {
                console.log("current region changed, setting clip editor region to ", arrangerObject.regionMixin.name.name);
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
                if (arrangerObject && arrangerObject.regionMixin) {
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
      text: qsTr("Timeline")
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
