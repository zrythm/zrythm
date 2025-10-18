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

  function beginObjectCreation(x: real, y: real): var {
    const track = getTrackAtY(y);
    if (!track) {
      return null;
    }

    const tickPosition = x / root.ruler.pxPerTick;

    const automationTrack = getAutomationTrackAtY(y);
    if (automationTrack) {
      console.log("Creating automation region");
      let region = objectCreator.addEmptyAutomationRegion(track, automationTrack, tickPosition);
      root.currentAction = Arranger.CreatingResizingR;
      const regionOwner = automationTrack.regions;
      root.selectSingleObject(regionOwner, regionOwner.rowCount() - 1);
      CursorManager.setResizeEndCursor();
      return region;
    }

    const trackLane = getTrackLaneAtY(y) as TrackLane;
    console.log("Timeline: beginObjectCreation", x, y, track, trackLane, automationTrack);

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

  function updateCursor() {
    let cursor = "default";

    switch (root.currentAction) {
    case Arranger.None:
      switch (root.tool.toolValue) {
      case Tool.Select:
        CursorManager.setPointerCursor();
        return;
      case Tool.Edit:
        CursorManager.setPencilCursor();
        return;
      case Tool.Cut:
        CursorManager.setCutCursor();
        return;
      case Tool.Eraser:
        CursorManager.setEraserCursor();
        return;
      case Tool.Ramp:
        CursorManager.setRampCursor();
        return;
      case Tool.Audition:
        CursorManager.setAuditionCursor();
        return;
      }
      break;
    case Arranger.StartingDeleteSelection:
    case Arranger.DeleteSelecting:
    case Arranger.StartingErasing:
    case Arranger.Erasing:
      CursorManager.setEraserCursor();
      return;
    case Arranger.StartingMovingCopy:
    case Arranger.MovingCopy:
      CursorManager.setCopyCursor();
      return;
    case Arranger.StartingMovingLink:
    case Arranger.MovingLink:
      CursorManager.setLinkCursor();
      return;
    case Arranger.StartingMoving:
    case Arranger.CreatingMoving:
    case Arranger.Moving:
      CursorManager.setClosedHandCursor();
      return;
    case Arranger.StartingPanning:
    case Arranger.Panning:
      CursorManager.setClosedHandCursor();
      return;
    case Arranger.StretchingL:
      CursorManager.setStretchStartCursor();
      return;
    case Arranger.ResizingL:
      CursorManager.setResizeStartCursor();
      return;
    case Arranger.ResizingLLoop:
      CursorManager.setResizeLoopStartCursor();
      return;
    case Arranger.ResizingLFade:
      CursorManager.setFadeInCursor();
      return;
    case Arranger.StretchingR:
      CursorManager.setStretchEndCursor();
      return;
    case Arranger.CreatingResizingR:
    case Arranger.CreatingResizingMovingR:
    case Arranger.ResizingR:
      CursorManager.setResizeEndCursor();
      return;
    case Arranger.ResizingRLoop:
      CursorManager.setResizeLoopEndCursor();
      return;
    case Arranger.ResizingRFade:
    case Arranger.ResizingUpFadeOut:
      CursorManager.setFadeOutCursor();
      return;
    case Arranger.ResizingUpFadeIn:
      CursorManager.setFadeInCursor();
      return;
    case Arranger.Autofilling:
      CursorManager.setBrushCursor();
      return;
    case Arranger.StartingSelection:
    case Arranger.Selecting:
      CursorManager.setPointerCursor();
      return;
    case Arranger.Renaming:
      cursor = "text";
      break;
    case Arranger.Cutting:
      CursorManager.setCutCursor();
      return;
    case Arranger.StartingAuditioning:
    case Arranger.Auditioning:
      CursorManager.setAuditionCursor();
      return;
    }

    CursorManager.setPointerCursor();
    return;
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

              delegate: Loader {
                id: scaleLoader

                required property var arrangerObject
                required property int index
                property var scaleObject: arrangerObject
                readonly property real scaleX: scaleObject.position.ticks * root.ruler.pxPerTick

                active: scaleX + 100 + Style.scrollLoaderBufferPx >= root.scrollX && scaleX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                asynchronous: true
                visible: status === Loader.Ready

                sourceComponent: Component {
                  ScaleObjectView {
                    id: scaleItem

                    arrangerObject: scaleLoader.scaleObject
                    isSelected: scaleSelelectionTracker.isSelected
                    pxPerTick: root.ruler.pxPerTick
                    track: trackDelegate.track
                    x: scaleLoader.scaleX

                    onHoveredChanged: {
                      root.handleObjectHover(scaleItem.hovered, scaleItem);
                    }
                    onSelectionRequested: function (mouse) {
                      root.handleObjectSelection(scalesRepeater.model, scaleLoader.index, mouse);
                    }
                  }
                }

                SelectionTracker {
                  id: scaleSelelectionTracker

                  modelIndex: {
                    root.unifiedObjectsModel.addSourceModel(scalesRepeater.model);
                    return root.unifiedObjectsModel.mapFromSource(scalesRepeater.model.index(scaleLoader.index, 0));
                  }
                  selectionModel: root.arrangerSelectionModel
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

              delegate: Loader {
                id: markerLoader

                required property var arrangerObject
                required property int index
                property var marker: arrangerObject
                readonly property real markerX: marker.position.ticks * root.ruler.pxPerTick

                active: markerX + 100 + Style.scrollLoaderBufferPx >= root.scrollX && markerX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                asynchronous: true
                visible: status === Loader.Ready

                sourceComponent: Component {
                  MarkerView {
                    id: markerItem

                    arrangerObject: markerLoader.marker
                    isSelected: markerSelelectionTracker.isSelected
                    pxPerTick: root.ruler.pxPerTick
                    track: trackDelegate.track
                    x: markerLoader.markerX

                    onHoveredChanged: {
                      root.handleObjectHover(markerItem.hovered, markerItem);
                    }
                    onSelectionRequested: function (mouse) {
                      root.handleObjectSelection(markersRepeater.model, markerLoader.index, mouse);
                    }
                  }
                }

                SelectionTracker {
                  id: markerSelelectionTracker

                  modelIndex: {
                    root.unifiedObjectsModel.addSourceModel(markersRepeater.model);
                    return root.unifiedObjectsModel.mapFromSource(markersRepeater.model.index(markerLoader.index, 0));
                  }
                  selectionModel: root.arrangerSelectionModel
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

              delegate: Loader {
                id: chordRegionLoader

                required property ArrangerObject arrangerObject
                required property int index
                readonly property ChordRegion region: arrangerObject as ChordRegion
                readonly property real regionEndX: regionX + regionWidth
                readonly property real regionWidth: region.bounds.length.ticks * root.ruler.pxPerTick
                readonly property real regionX: region.position.ticks * root.ruler.pxPerTick

                active: regionEndX + Style.scrollLoaderBufferPx >= root.scrollX && regionX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                asynchronous: true
                visible: status === Loader.Ready

                sourceComponent: Component {
                  ChordRegionView {
                    id: chordRegionItem

                    arrangerObject: chordRegionLoader.region
                    clipEditor: root.clipEditor
                    height: chordRegionsRepeater.height
                    isSelected: chordRegionSelectionTracker.isSelected
                    pxPerTick: root.ruler.pxPerTick
                    track: trackDelegate.track
                    width: chordRegionLoader.regionWidth
                    x: chordRegionLoader.regionX

                    onHoveredChanged: {
                      root.handleObjectHover(hovered, chordRegionItem);
                    }
                    onSelectionRequested: function (mouse) {
                      root.handleObjectSelection(chordRegionsRepeater.model, chordRegionLoader.index, mouse);
                    }
                  }
                }

                SelectionTracker {
                  id: chordRegionSelectionTracker

                  modelIndex: {
                    root.unifiedObjectsModel.addSourceModel(chordRegionsRepeater.model);
                    return root.unifiedObjectsModel.mapFromSource(chordRegionsRepeater.model.index(chordRegionLoader.index, 0));
                  }
                  selectionModel: root.arrangerSelectionModel
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

                delegate: Loader {
                  id: mainTrackRegionLoader

                  required property var arrangerObject
                  required property int index
                  property var region: arrangerObject
                  readonly property real regionEndX: regionX + regionWidth
                  readonly property real regionHeight: mainTrackLanedRegionsLoader.height
                  readonly property real regionWidth: region.bounds.length.ticks * root.ruler.pxPerTick
                  readonly property real regionX: region.position.ticks * root.ruler.pxPerTick
                  readonly property var trackLane: mainTrackLaneRegionsRepeater.trackLane

                  active: regionEndX + Style.scrollLoaderBufferPx >= root.scrollX && regionX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                  asynchronous: true
                  height: regionHeight
                  sourceComponent: region.type === ArrangerObject.MidiRegion ? midiRegionComponent : audioRegionComponent
                  visible: status === Loader.Ready

                  SelectionTracker {
                    id: mainTrackRegionSelectionTracker

                    modelIndex: {
                      root.unifiedObjectsModel.addSourceModel(mainTrackLaneRegionsRepeater.model);
                      return root.unifiedObjectsModel.mapFromSource(mainTrackLaneRegionsRepeater.model.index(mainTrackRegionLoader.index, 0));
                    }
                    selectionModel: root.arrangerSelectionModel
                  }

                  Component {
                    id: midiRegionComponent

                    MidiRegionView {
                      id: mainMidiRegionItem

                      arrangerObject: mainTrackRegionLoader.region
                      clipEditor: root.clipEditor
                      height: mainTrackRegionLoader.regionHeight
                      isSelected: mainTrackRegionSelectionTracker.isSelected
                      lane: mainTrackLaneRegionsRepeater.trackLane
                      pxPerTick: root.ruler.pxPerTick
                      track: trackDelegate.track
                      width: mainTrackRegionLoader.regionWidth
                      x: mainTrackRegionLoader.regionX

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

                      arrangerObject: mainTrackRegionLoader.region
                      clipEditor: root.clipEditor
                      height: mainTrackRegionLoader.regionHeight
                      isSelected: mainTrackRegionSelectionTracker.isSelected
                      lane: mainTrackLaneRegionsRepeater.trackLane
                      pxPerTick: root.ruler.pxPerTick
                      track: trackDelegate.track
                      width: mainTrackRegionLoader.regionWidth
                      x: mainTrackRegionLoader.regionX

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

                Loader {
                  id: laneRegionLoader

                  required property ArrangerObject arrangerObject
                  required property int index
                  property var region: arrangerObject
                  readonly property real regionEndX: regionX + regionWidth
                  readonly property real regionHeight: trackLane.height
                  readonly property real regionWidth: region.bounds.length.ticks * root.ruler.pxPerTick
                  readonly property real regionX: region.position.ticks * root.ruler.pxPerTick
                  readonly property var trackLane: laneItem.trackLane

                  active: regionEndX + Style.scrollLoaderBufferPx >= root.scrollX && regionX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                  asynchronous: true
                  height: regionHeight
                  sourceComponent: region.type === ArrangerObject.MidiRegion ? laneMidiRegionComponent : laneAudioRegionComponent
                  visible: status === Loader.Ready

                  SelectionTracker {
                    id: laneRegionSelectionTracker

                    modelIndex: {
                      root.unifiedObjectsModel.addSourceModel(laneMidiRegionsRepeater.model);
                      return root.unifiedObjectsModel.mapFromSource(laneMidiRegionsRepeater.model.index(laneRegionLoader.index, 0));
                    }
                    selectionModel: root.arrangerSelectionModel
                  }

                  Component {
                    id: laneMidiRegionComponent

                    MidiRegionView {
                      id: laneMidiRegionItem

                      arrangerObject: laneRegionLoader.region
                      clipEditor: root.clipEditor
                      height: laneItem.trackLane.height
                      isSelected: laneRegionSelectionTracker.isSelected
                      lane: laneItem.trackLane
                      pxPerTick: root.ruler.pxPerTick
                      track: trackDelegate.track
                      width: laneRegionLoader.regionWidth
                      x: laneRegionLoader.regionX

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

                      arrangerObject: laneRegionLoader.region
                      clipEditor: root.clipEditor
                      height: laneItem.trackLane.height
                      isSelected: laneRegionSelectionTracker.isSelected
                      lane: laneItem.trackLane
                      pxPerTick: root.ruler.pxPerTick
                      track: trackDelegate.track
                      width: laneRegionLoader.regionWidth
                      x: laneRegionLoader.regionX

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

                delegate: Loader {
                  id: automationRegionLoader

                  required property ArrangerObject arrangerObject
                  readonly property AutomationTrack automationTrack: automationTrackItem.automationTrack
                  required property int index
                  readonly property AutomationRegion region: arrangerObject as AutomationRegion
                  readonly property real regionEndX: regionX + regionWidth
                  readonly property real regionHeight: automationTrackItem.height
                  readonly property real regionWidth: region.bounds.length.ticks * root.ruler.pxPerTick
                  readonly property real regionX: region.position.ticks * root.ruler.pxPerTick

                  active: regionEndX + Style.scrollLoaderBufferPx >= root.scrollX && regionX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                  asynchronous: true
                  height: regionHeight
                  visible: status === Loader.Ready

                  sourceComponent: AutomationRegionView {
                    id: automationRegionItem

                    arrangerObject: automationRegionLoader.region
                    automationTrack: automationRegionLoader.automationTrack
                    clipEditor: root.clipEditor
                    height: automationTrackItem.automationTrackHolder.height
                    isSelected: automationRegionSelectionTracker.isSelected
                    pxPerTick: root.ruler.pxPerTick
                    track: trackDelegate.track
                    width: automationRegionLoader.regionWidth
                    x: automationRegionLoader.regionX

                    onHoveredChanged: {
                      root.handleObjectHover(hovered, automationRegionItem);
                    }
                    onSelectionRequested: function (mouse) {
                      root.handleObjectSelection(automationRegionsRepeater.model, automationRegionLoader.index, mouse);
                    }
                  }

                  SelectionTracker {
                    id: automationRegionSelectionTracker

                    modelIndex: {
                      root.unifiedObjectsModel.addSourceModel(automationRegionsRepeater.model);
                      return root.unifiedObjectsModel.mapFromSource(automationRegionsRepeater.model.index(automationRegionLoader.index, 0));
                    }
                    selectionModel: root.arrangerSelectionModel
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
