// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import ZrythmArrangement
import Zrythm

Arranger {
  id: root

  required property bool pinned
  required property var timeline
  required property Tracklist tracklist

  function beginObjectCreation(coordinates: point): ArrangerObject {
    const track = getTrackAtY(coordinates.y);
    if (!track) {
      return null;
    }

    const tickPosition = coordinates.x / root.ruler.pxPerTick;

    const automationTrack = getAutomationTrackAtY(coordinates.y);
    if (automationTrack) {
      console.log("Creating automation region");
      let region = objectCreator.addEmptyAutomationRegion(track, automationTrack, tickPosition);
      root.currentAction = Arranger.CreatingResizingR;
      const regionOwner = automationTrack.regions;
      root.selectSingleObject(regionOwner, regionOwner.rowCount() - 1);
      CursorManager.setResizeEndCursor();
      return region;
    }

    const trackLane = getTrackLaneAtY(coordinates.y) as TrackLane;
    console.log("Timeline: beginObjectCreation", coordinates, track, trackLane, automationTrack);

    switch (track.type) {
    case Track.Chord:
      {
        console.log("creating chord region");
        let region = objectCreator.addEmptyChordRegion(track, tickPosition);
        root.currentAction = Arranger.CreatingResizingR;
        const regionOwner = (track as ChordTrack).chordRegions;
        root.selectSingleObject(regionOwner, regionOwner.rowCount() - 1);
        CursorManager.setResizeEndCursor();
        return region;
      }
    case Track.Marker:
      console.log("creating marker", Track.Marker, ArrangerObject.Marker);
      root.undoStack.beginMacro("Create Marker");
      let marker = objectCreator.addMarker(Marker.Custom, track, qsTr("Custom Marker"), tickPosition);
      root.currentAction = Arranger.CreatingMoving;
      const markerTrack = track as MarkerTrack;
      root.selectSingleObject(markerTrack.markers, markerTrack.markers.rowCount() - 1);
      CursorManager.setClosedHandCursor();
      return marker;
    case Track.Midi:
    case Track.Instrument:
      console.log("creating midi region", track.lanes.getFirstLane());
      let region = objectCreator.addEmptyMidiRegion(track, trackLane ? trackLane : track.lanes.getFirstLane(), tickPosition);
      root.currentAction = Arranger.CreatingResizingR;
      const regionOwner = trackLane ? trackLane.midiRegions : track.lanes.getFirstLane().midiRegions;
      root.selectSingleObject(regionOwner, regionOwner.rowCount() - 1);
      CursorManager.setResizeEndCursor();
      return region;
    default:
      return null;
    }
    return null;
  }

  function getAutomationTrackAtY(y: real): AutomationTrack {
    y += tracksListView.contentY;
    const trackItem = tracksListView.itemAt(0, y);
    if (!trackItem) {
      return null;
    }
    const track = trackItem.track as Track;
    if (track.automationTracklist === null || !track.automationTracklist.automationVisible) {
      return null;
    }

    y -= trackItem.y;

    // Get the automation loader instance
    const columnLayout = trackItem.children[0]; // trackSectionRows
    const loader = columnLayout.children[2]; // automationLoader
    if (!loader?.item) {
      return null;
    }

    // Get relative Y position within the automation tracks list
    const automationListY = y - loader.y;
    const automationItem = loader.item.itemAt(0, automationListY);
    console.log("Timeline: getAutomationTrackAtY", y, trackItem, loader, automationListY, automationItem);
    return automationItem?.automationTrack ?? null;
  }

  function getObjectY(obj: ArrangerObject): real {
    return 0;
  }

  function getTrackAtY(y: real): Track {
    const item = tracksListView.itemAt(0, y + tracksListView.contentY);
    return item?.track ?? null;
  }

  function getTrackLaneAtY(y: real): TrackLane {
    // Get the track delegate
    const trackItem = tracksListView.itemAt(0, y + tracksListView.contentY);
    if (!trackItem) {
      return null;
    }
    const track = trackItem.track as Track;
    if (track.lanes === null || !track.lanes.lanesVisible) {
      return null;
    }

    // Make y relative to trackItem
    const relativeY = y + tracksListView.contentY - trackItem.y;

    // Get ColumnLayout (trackSectionRows) and laneRegionsLoader
    const columnLayout = trackItem.children[0];
    const laneLoader = columnLayout.children[1]; // laneRegionsLoader is second child
    if (!laneLoader?.item) {
      return null;
    }

    // Get relative Y position within the lanes list
    const laneListY = relativeY - laneLoader.y;
    const laneItem = laneLoader.item.itemAt(0, laneListY);
    console.log("Timeline: getTrackLaneAtY", relativeY, trackItem, laneLoader, laneListY, laneItem);
    return laneItem?.trackLane ?? null;
  }

  function moveSelectionsY(dy: real, prevY: real) {
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
  }

  editorSettings: timeline.editorSettings
  enableYScroll: !pinned
  scrollView.ScrollBar.horizontal.policy: pinned ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded

  content: ListView {
    id: tracksListView

    anchors.fill: parent
    interactive: false

    delegate: Item {
      id: trackDelegate

      required property Track track

      height: track.fullVisibleHeight
      width: tracksListView.width

      TextMetrics {
        id: arrangerObjectTextMetrics

        font: Style.arrangerObjectTextFont
        text: "Some text"
      }

      ColumnLayout {
        id: trackSectionRows

        anchors.fill: parent
        spacing: 0

        Item {
          id: mainTrackObjectsContainer

          Layout.fillWidth: true
          Layout.maximumHeight: trackDelegate.track.height
          Layout.minimumHeight: trackDelegate.track.height

          Loader {
            id: scalesLoader

            active: trackDelegate.track.type === Track.Chord
            height: arrangerObjectTextMetrics.height + 2 * Style.buttonPadding
            visible: active
            width: parent.width
            y: trackDelegate.track.height - height

            sourceComponent: Repeater {
              id: scalesRepeater

              readonly property ChordTrack track: trackDelegate.track as ChordTrack

              model: track.scaleObjects

              delegate: ArrangerObjectLoader {
                id: scaleLoader

                arrangerSelectionModel: root.arrangerSelectionModel
                model: scalesRepeater.model
                pxPerTick: root.ruler.pxPerTick
                scrollViewWidth: root.scrollViewWidth
                scrollX: root.scrollX
                unifiedObjectsModel: root.unifiedObjectsModel

                sourceComponent: Component {
                  ScaleObjectView {
                    id: scaleItem

                    arrangerObject: scaleLoader.arrangerObject
                    isSelected: scaleLoader.selectionTracker.isSelected
                    track: trackDelegate.track

                    onHoveredChanged: {
                      root.handleObjectHover(scaleItem.hovered, scaleItem);
                    }
                    onSelectionRequested: function (mouse) {
                      root.handleObjectSelection(scalesRepeater.model, scaleLoader.index, mouse);
                    }
                  }
                }
              }
            }
          }

          Loader {
            id: markersLoader

            active: trackDelegate.track.type === Track.Marker
            height: arrangerObjectTextMetrics.height + 2 * Style.buttonPadding
            visible: active
            width: parent.width
            y: trackDelegate.track.height - height

            sourceComponent: Repeater {
              id: markersRepeater

              readonly property MarkerTrack track: trackDelegate.track as MarkerTrack

              model: track.markers

              delegate: ArrangerObjectLoader {
                id: markerLoader

                arrangerSelectionModel: root.arrangerSelectionModel
                model: markersRepeater.model
                pxPerTick: root.ruler.pxPerTick
                scrollViewWidth: root.scrollViewWidth
                scrollX: root.scrollX
                unifiedObjectsModel: root.unifiedObjectsModel

                sourceComponent: Component {
                  MarkerView {
                    id: markerItem

                    arrangerObject: markerLoader.arrangerObject
                    isSelected: markerLoader.selectionTracker.isSelected
                    track: trackDelegate.track

                    onHoveredChanged: {
                      root.handleObjectHover(markerItem.hovered, markerItem);
                    }
                    onSelectionRequested: function (mouse) {
                      root.handleObjectSelection(markersRepeater.model, markerLoader.index, mouse);
                    }
                  }
                }
              }
            }
          }

          Loader {
            id: chordRegionsLoader

            active: trackDelegate.track.type === Track.Chord
            height: trackDelegate.track.height - scalesLoader.height
            visible: active
            width: parent.width
            y: 0

            sourceComponent: Repeater {
              id: chordRegionsRepeater

              readonly property ChordTrack track: trackDelegate.track as ChordTrack

              model: track.chordRegions

              delegate: ArrangerObjectLoader {
                id: chordRegionLoader

                arrangerSelectionModel: root.arrangerSelectionModel
                model: chordRegionsRepeater.model
                pxPerTick: root.ruler.pxPerTick
                scrollViewWidth: root.scrollViewWidth
                scrollX: root.scrollX
                unifiedObjectsModel: root.unifiedObjectsModel

                sourceComponent: Component {
                  ChordRegionView {
                    id: chordRegionItem

                    arrangerObject: chordRegionLoader.arrangerObject
                    clipEditor: root.clipEditor
                    height: chordRegionsRepeater.height
                    isSelected: chordRegionLoader.selectionTracker.isSelected
                    track: trackDelegate.track

                    onHoveredChanged: {
                      root.handleObjectHover(hovered, chordRegionItem);
                    }
                    onSelectionRequested: function (mouse) {
                      root.handleObjectSelection(chordRegionsRepeater.model, chordRegionLoader.index, mouse);
                    }
                  }
                }
              }
            }
          }

          Loader {
            id: mainTrackLanedRegionsLoader

            active: trackDelegate.track.lanes !== null
            anchors.fill: parent
            visible: active

            sourceComponent: Repeater {
              id: mainTrackLanesRepeater

              model: trackDelegate.track.lanes

              delegate: Repeater {
                id: mainTrackLaneRegionsRepeater

                required property TrackLane trackLane

                model: trackDelegate.track.type === Track.Audio ? trackLane.audioRegions : trackLane.midiRegions

                delegate: ArrangerObjectLoader {
                  id: mainTrackRegionLoader

                  arrangerSelectionModel: root.arrangerSelectionModel
                  model: mainTrackLaneRegionsRepeater.model
                  pxPerTick: root.ruler.pxPerTick
                  scrollViewWidth: root.scrollViewWidth
                  scrollX: root.scrollX
                  unifiedObjectsModel: root.unifiedObjectsModel
                  height: mainTrackLanedRegionsLoader.height

                  sourceComponent: mainTrackRegionLoader.arrangerObject.type === ArrangerObject.MidiRegion ? midiRegionComponent : audioRegionComponent

                  Component {
                    id: midiRegionComponent

                    MidiRegionView {
                      id: mainMidiRegionItem

                      arrangerObject: mainTrackRegionLoader.arrangerObject
                      clipEditor: root.clipEditor
                      isSelected: mainTrackRegionLoader.selectionTracker.isSelected
                      lane: mainTrackLaneRegionsRepeater.trackLane
                      track: trackDelegate.track

                      onHoveredChanged: {
                        root.handleObjectHover(hovered, mainMidiRegionItem);
                      }
                      onSelectionRequested: function (mouse) {
                        root.handleObjectSelection(mainTrackLaneRegionsRepeater.model, mainTrackRegionLoader.index, mouse);
                      }
                    }
                  }

                  Component {
                    id: audioRegionComponent

                    AudioRegionView {
                      id: mainAudioRegionItem

                      arrangerObject: mainTrackRegionLoader.arrangerObject
                      clipEditor: root.clipEditor
                      height: mainTrackRegionLoader.height
                      isSelected: mainTrackRegionLoader.selectionTracker.isSelected
                      lane: mainTrackLaneRegionsRepeater.trackLane
                      track: trackDelegate.track

                      onHoveredChanged: {
                        root.handleObjectHover(hovered, mainAudioRegionItem);
                      }
                      onSelectionRequested: function (mouse) {
                        root.handleObjectSelection(mainTrackLaneRegionsRepeater.model, mainTrackRegionLoader.index, mouse);
                      }
                    }
                  }
                }
              }
            }
          }
        }

        Loader {
          id: laneRegionsLoader

          Layout.fillWidth: true
          Layout.maximumHeight: Layout.preferredHeight
          Layout.minimumHeight: Layout.preferredHeight
          Layout.preferredHeight: item ? item.contentHeight : 0
          active: trackDelegate.track.lanes && trackDelegate.track.lanes.lanesVisible
          visible: active

          sourceComponent: ListView {
            id: lanesListView

            anchors.fill: parent
            clip: true
            interactive: false
            model: trackDelegate.track.lanes
            orientation: Qt.Vertical
            spacing: 0

            delegate: Item {
              id: laneItem

              required property TrackLane trackLane

              height: trackLane.height
              width: ListView.view.width

              Component {
                id: laneRegionLoaderComponent

                ArrangerObjectLoader {
                  id: laneRegionLoader

                  arrangerSelectionModel: root.arrangerSelectionModel
                  model: arrangerObject.type === ArrangerObject.MidiRegion ? laneItem.trackLane.midiRegions : laneItem.trackLane.audioRegions
                  pxPerTick: root.ruler.pxPerTick
                  scrollViewWidth: root.scrollViewWidth
                  scrollX: root.scrollX
                  unifiedObjectsModel: root.unifiedObjectsModel
                  height: laneItem.trackLane.height

                  sourceComponent: laneRegionLoader.arrangerObject.type === ArrangerObject.MidiRegion ? laneMidiRegionComponent : laneAudioRegionComponent

                  Component {
                    id: laneMidiRegionComponent

                    MidiRegionView {
                      id: laneMidiRegionItem

                      arrangerObject: laneRegionLoader.arrangerObject
                      clipEditor: root.clipEditor
                      isSelected: laneRegionLoader.selectionTracker.isSelected
                      lane: laneItem.trackLane
                      track: trackDelegate.track

                      onHoveredChanged: {
                        root.handleObjectHover(hovered, laneMidiRegionItem);
                      }
                      onSelectionRequested: function (mouse) {
                        root.handleObjectSelection(laneMidiRegionsRepeater.model, laneRegionLoader.index, mouse);
                      }
                    }
                  }

                  Component {
                    id: laneAudioRegionComponent

                    AudioRegionView {
                      id: laneAudioRegionItem

                      arrangerObject: laneRegionLoader.arrangerObject
                      clipEditor: root.clipEditor
                      isSelected: laneRegionLoader.selectionTracker.isSelected
                      lane: laneItem.trackLane
                      track: trackDelegate.track

                      onHoveredChanged: {
                        root.handleObjectHover(hovered, laneAudioRegionItem);
                      }
                      onSelectionRequested: function (mouse) {
                        root.handleObjectSelection(laneAudioRegionsRepeater.model, laneRegionLoader.index, mouse);
                      }
                    }
                  }
                }
              }

              Repeater {
                id: laneMidiRegionsRepeater

                anchors.fill: parent
                delegate: laneRegionLoaderComponent
                model: laneItem.trackLane.midiRegions
              }

              Repeater {
                id: laneAudioRegionsRepeater

                anchors.fill: parent
                delegate: laneRegionLoaderComponent
                model: laneItem.trackLane.audioRegions
              }
            }
          }
        }

        Loader {
          id: automationLoader

          readonly property AutomationTracklist automationTracklist: trackDelegate.track.automationTracklist

          Layout.fillWidth: true
          Layout.maximumHeight: Layout.preferredHeight
          Layout.minimumHeight: Layout.preferredHeight
          Layout.preferredHeight: item ? item.contentHeight : 0
          active: trackDelegate.track.automationTracklist !== null && automationTracklist.automationVisible
          visible: active

          sourceComponent: ListView {
            id: automationTracksListView

            anchors.fill: parent
            clip: true
            interactive: false
            orientation: Qt.Vertical
            spacing: 0

            delegate: Item {
              id: automationTrackItem

              readonly property var automationTrack: automationTrackHolder.automationTrack
              required property var automationTrackHolder

              height: automationTrackHolder.height
              width: ListView.view.width

              Repeater {
                id: automationRegionsRepeater

                anchors.fill: parent
                model: automationTrackItem.automationTrack.regions

                delegate: ArrangerObjectLoader {
                  id: automationRegionLoader

                  arrangerSelectionModel: root.arrangerSelectionModel
                  model: automationRegionsRepeater.model
                  pxPerTick: root.ruler.pxPerTick
                  scrollViewWidth: root.scrollViewWidth
                  scrollX: root.scrollX
                  unifiedObjectsModel: root.unifiedObjectsModel
                  height: automationTrackItem.height

                  sourceComponent: Component {
                    AutomationRegionView {
                      id: automationRegionItem

                      arrangerObject: automationRegionLoader.arrangerObject
                      automationTrack: automationTrackItem.automationTrack
                      clipEditor: root.clipEditor
                      isSelected: automationRegionLoader.selectionTracker.isSelected
                      track: trackDelegate.track

                      onHoveredChanged: {
                        root.handleObjectHover(hovered, automationRegionItem);
                      }
                      onSelectionRequested: function (mouse) {
                        root.handleObjectSelection(automationRegionsRepeater.model, automationRegionLoader.index, mouse);
                      }
                    }
                  }
                }
              }
            }
            model: AutomationTracklistProxyModel {
              showOnlyCreated: true
              showOnlyVisible: true
              sourceModel: automationLoader.automationTracklist
            }
          }
        }
      }
    }
    model: TrackFilterProxyModel {
      sourceModel: root.tracklist.collection

      Component.onCompleted: {
        addVisibilityFilter(true);
        addPinnedFilter(root.pinned);
      }
    }
  }
}
