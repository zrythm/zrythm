// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Arranger {
    id: root

    required property bool pinned
    required property var timeline
    required property var tracklist

    function getTrackAtY(y: real): var {
        const item = tracksListView.itemAt(0, y + tracksListView.contentY)
        return item?.track ?? null
    }

    function getAutomationTrackAtY(y: real): var {
        y += tracksListView.contentY;
        const trackItem = tracksListView.itemAt(0, y);
        if (!trackItem?.track?.isAutomatable || !trackItem?.track?.automationVisible) {
            return null;
        }

        y -= trackItem.y;

        // Get the automation loader instance
        const columnLayout = trackItem.children[0] // trackSectionRows
        const loader = columnLayout.children[2] // automationLoader
        if (!loader?.item) {
            return null;
        }

        // Get relative Y position within the automation tracks list
        const automationListY = y - loader.y;
        const automationItem = loader.item.itemAt(0, automationListY);
        console.log("Timeline: getAutomationTrackAtY", y, trackItem, loader, automationListY, automationItem);
        return automationItem?.automationTrack ?? null;
    }

   function getTrackLaneAtY(y: real): var {
        // Get the track delegate
        const trackItem = tracksListView.itemAt(0, y + tracksListView.contentY)
        if (!trackItem?.track?.hasLanes || !trackItem?.track?.lanesVisible) {
            return null
        }

        // Make y relative to trackItem
        const relativeY = y + tracksListView.contentY - trackItem.y

        // Get ColumnLayout (trackSectionRows) and laneRegionsLoader
        const columnLayout = trackItem.children[0]
        const laneLoader = columnLayout.children[1] // laneRegionsLoader is second child
        if (!laneLoader?.item) {
            return null
        }

        // Get relative Y position within the lanes list
        const laneListY = relativeY - laneLoader.y
        const laneItem = laneLoader.item.itemAt(0, laneListY)
        console.log("Timeline: getTrackLaneAtY", relativeY, trackItem, laneLoader, laneListY, laneItem);
        return laneItem?.trackLane ?? null
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

    function beginObjectCreation(x: real, y: real): var {
        const track = getTrackAtY(y);
        if (!track) {
            return null;
        }

        const automationTrack = getAutomationTrackAtY(y);
        if (!automationTrack) {
        }
        const trackLane = getTrackLaneAtY(y);
        if (!trackLane) {
        }
        console.log("Timeline: beginObjectCreation", x, y, track, trackLane, automationTrack);

        switch (track.type) {
            case 3:
                console.log("creating chord");
                break;
            case 4:
                console.log("creating marker");
                break;
            case 8:
                console.log("creating midi region", track.lanes.getFirstLane());
                let region = objectFactory.addEmptyMidiRegion(trackLane ? trackLane : track.lanes.getFirstLane(), x / root.ruler.pxPerTick);
                root.currentAction = Arranger.CreatingResizingR;
                root.setObjectSnapshotsAtStart();
                CursorManager.setResizeEndCursor();
                root.actionObject = region;
                return region;
            default:
                return null;
        }
        return null;
    }

    editorSettings: timeline
    enableYScroll: !pinned
    scrollView.ScrollBar.horizontal.policy: pinned ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded

    content: ListView {
        id: tracksListView

        anchors.fill: parent
        interactive: false

        model: TrackFilterProxyModel {
            sourceModel: tracklist
            Component.onCompleted: {
                addVisibilityFilter(true);
                addPinnedFilter(root.pinned);
            }
        }

        delegate: Item {
            id: trackDelegate

            required property var track

            width: tracksListView.width
            height: track.fullVisibleHeight

            TextMetrics {
                id: arrangerObjectTextMetrics

                text: "Some text"
                font: Style.arrangerObjectTextFont
            }

            ColumnLayout {
                id: trackSectionRows

                anchors.fill: parent
                spacing: 0

                Item {
                    id: mainTrackObjectsContainer

                    Layout.fillWidth: true
                    Layout.minimumHeight: trackDelegate.track.height
                    Layout.maximumHeight: trackDelegate.track.height

                    Loader {
                        id: scalesLoader

                        width: parent.width
                        y: trackDelegate.track.height - height
                        height: arrangerObjectTextMetrics.height + 2 * Style.buttonPadding
                        active: trackDelegate.track.type === 3
                        visible: active

                        sourceComponent: Repeater {
                            id: scalesRepeater

                            model: trackDelegate.track.scaleObjects

                            delegate: Loader {
                                id: scaleLoader

                                required property var arrangerObject
                                property var scaleObject: arrangerObject
                                readonly property real scaleX: scaleObject.position.ticks * root.ruler.pxPerTick

                                active: scaleX + 100 + Style.scrollLoaderBufferPx >= root.scrollX && scaleX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                                visible: status === Loader.Ready
                                asynchronous: true

                                sourceComponent: Component {
                                    ScaleObject {
                                        id: scaleItem

                                        arrangerObject: scaleLoader.scaleObject
                                        track: trackDelegate.track
                                        ruler: root.ruler
                                        x: scaleLoader.scaleX
                                    }

                                }

                            }

                        }

                    }

                    Loader {
                        id: markersLoader

                        width: parent.width
                        y: trackDelegate.track.height - height
                        height: arrangerObjectTextMetrics.height + 2 * Style.buttonPadding
                        active: trackDelegate.track.type === 4
                        visible: active

                        sourceComponent: Repeater {
                            id: markersRepeater

                            model: trackDelegate.track.markers

                            delegate: Loader {
                                id: markerLoader

                                required property var arrangerObject
                                property var marker: arrangerObject
                                readonly property real markerX: marker.position.ticks * root.ruler.pxPerTick

                                active: markerX + 100 + Style.scrollLoaderBufferPx >= root.scrollX && markerX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                                visible: status === Loader.Ready
                                asynchronous: true

                                sourceComponent: Component {
                                    Marker {
                                        id: markerItem

                                        arrangerObject: markerLoader.marker
                                        track: trackDelegate.track
                                        ruler: root.ruler
                                        x: markerLoader.markerX
                                    }

                                }

                            }

                        }

                    }

                    Loader {
                        id: chordRegionsLoader

                        active: track.type === 3
                        visible: active
                        y: 0
                        width: parent.width
                        height: trackDelegate.track.height - scalesLoader.height

                        sourceComponent: Repeater {
                            id: chordRegionsRepeater

                            model: track.regions

                            delegate: Loader {
                                id: chordRegionLoader

                                required property var region
                                readonly property real chordRegionX: region.position.ticks * root.ruler.pxPerTick
                                readonly property real chordRegionEndX: region.endPosition.ticks * root.ruler.pxPerTick
                                readonly property real chordRegionWidth: chordRegionEndX - chordRegionX

                                active: chordRegionEndX + Style.scrollLoaderBufferPx >= root.scrollX && chordRegionX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                                visible: status === Loader.Ready
                                asynchronous: true

                                sourceComponent: Component {
                                    ChordRegion {
                                        ruler: root.ruler
                                        track: trackDelegate.track
                                        clipEditor: root.clipEditor
                                        arrangerObject: chordRegionLoader.region
                                        x: chordRegionLoader.chordRegionX
                                        width: chordRegionLoader.chordRegionWidth
                                        height: chordRegionsRepeater.height
                                    }

                                }

                            }

                        }

                    }

                    Loader {
                        id: mainTrackLanedRegionsLoader

                        active: track.hasLanes
                        anchors.fill: parent
                        visible: active

                        Component {
                            id: midiRegionComponent

                            MidiRegion {
                                ruler: root.ruler
                                track: trackDelegate.track
                                clipEditor: root.clipEditor
                                arrangerObject: region
                                lane: trackLane
                                height: regionHeight
                                x: regionX
                                width: regionWidth
                            }

                        }

                        Component {
                            id: audioRegionComponent

                            AudioRegion {
                                ruler: root.ruler
                                track: trackDelegate.track
                                clipEditor: root.clipEditor
                                arrangerObject: region
                                lane: trackLane
                                height: regionHeight
                                x: regionX
                                width: regionWidth
                            }

                        }

                        sourceComponent: Repeater {
                            id: mainTrackLanesRepeater

                            model: track.lanes

                            delegate: Repeater {
                                id: mainTrackLaneRegionsRepeater

                                required property var trackLane

                                model: trackLane.regions

                                delegate: Loader {
                                    id: mainTrackRegionLoader

                                    readonly property var trackLane: mainTrackLaneRegionsRepeater.trackLane
                                    required property var arrangerObject
                                    property var region: arrangerObject
                                    readonly property real regionX: region.position.ticks * root.ruler.pxPerTick
                                    readonly property real regionEndX: region.endPosition.ticks * root.ruler.pxPerTick
                                    readonly property real regionWidth: regionEndX - regionX
                                    readonly property real regionHeight: mainTrackLanedRegionsLoader.height

                                    height: regionHeight
                                    active: regionEndX + Style.scrollLoaderBufferPx >= root.scrollX && regionX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                                    visible: status === Loader.Ready
                                    asynchronous: true
                                    sourceComponent: region.regionType === 0 ? midiRegionComponent : audioRegionComponent
                                }

                            }

                        }

                    }

                }

                Loader {
                    id: laneRegionsLoader

                    active: track.hasLanes && track.lanesVisible
                    Layout.preferredHeight: item ? item.contentHeight : 0
                    Layout.minimumHeight: Layout.preferredHeight
                    Layout.maximumHeight: Layout.preferredHeight
                    Layout.fillWidth: true
                    visible: active

                    Component {
                        id: laneMidiRegionComponent

                        MidiRegion {
                            ruler: root.ruler
                            track: trackDelegate.track
                            clipEditor: root.clipEditor
                            arrangerObject: region
                            lane: trackLane
                            height: trackLane.height
                            x: regionX
                            width: regionWidth
                        }

                    }

                    Component {
                        id: laneAudioRegionComponent

                        AudioRegion {
                            ruler: root.ruler
                            track: trackDelegate.track
                            clipEditor: root.clipEditor
                            arrangerObject: region
                            lane: trackLane
                            height: trackLane.height
                            x: regionX
                            width: regionWidth
                        }

                    }

                    sourceComponent: ListView {
                        id: lanesListView

                        anchors.fill: parent
                        clip: true
                        interactive: false
                        spacing: 0
                        orientation: Qt.Vertical
                        model: track.lanes

                        delegate: Item {
                            id: laneItem

                            required property var trackLane

                            width: ListView.view.width
                            height: trackLane.height

                            Repeater {
                                id: laneRegionsRepeater

                                anchors.fill: parent
                                model: laneItem.trackLane.regions

                                delegate: Loader {
                                    id: laneRegionLoader

                                    readonly property var trackLane: laneItem.trackLane
                                    required property var region
                                    readonly property real regionX: region.position.ticks * root.ruler.pxPerTick
                                    readonly property real regionEndX: region.endPosition.ticks * root.ruler.pxPerTick
                                    readonly property real regionWidth: regionEndX - regionX
                                    readonly property real regionHeight: parent.height

                                    height: regionHeight
                                    active: regionEndX + Style.scrollLoaderBufferPx >= root.scrollX && regionX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                                    visible: status === Loader.Ready
                                    asynchronous: true
                                    sourceComponent: region.regionType === 0 ? laneMidiRegionComponent : laneAudioRegionComponent
                                }

                            }

                        }

                    }

                }

                Loader {
                    id: automationLoader

                    active: track.isAutomatable && track.automationVisible
                    Layout.preferredHeight: item ? item.contentHeight : 0
                    Layout.minimumHeight: Layout.preferredHeight
                    Layout.maximumHeight: Layout.preferredHeight
                    Layout.fillWidth: true
                    visible: active

                    Component {
                        id: automationRegion

                        AutomationRegion {
                            ruler: root.ruler
                            track: trackDelegate.track
                            arrangerObject: region
                            height: automationTrack.height
                            x: regionX
                            width: regionWidth
                        }

                    }

                    sourceComponent: ListView {
                        id: automationTracksListView

                        anchors.fill: parent
                        clip: true
                        interactive: false
                        spacing: 0
                        orientation: Qt.Vertical

                        model: AutomationTracklistProxyModel {
                            sourceModel: track.automationTracks
                            showOnlyVisible: true
                            showOnlyCreated: true
                        }

                        delegate: Item {
                            id: automationTrackItem

                            required property var automationTrack

                            width: ListView.view.width
                            height: automationTrack.height

                            Repeater {
                                id: automationRegionsRepeater

                                anchors.fill: parent
                                model: automationTrackItem.automationTrack.regions

                                delegate: Loader {
                                    id: automationRegionLoader

                                    readonly property var automationTrack: automationTrackItem.automationTrack
                                    required property var region
                                    readonly property real regionX: region.position.ticks * root.ruler.pxPerTick
                                    readonly property real regionEndX: region.endPosition.ticks * root.ruler.pxPerTick
                                    readonly property real regionWidth: regionEndX - regionX
                                    readonly property real regionHeight: parent.height

                                    height: regionHeight
                                    active: regionEndX + Style.scrollLoaderBufferPx >= root.scrollX && regionX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
                                    visible: status === Loader.Ready
                                    asynchronous: true

                                    sourceComponent: AutomationRegion {
                                        ruler: root.ruler
                                        track: trackDelegate.track
                                        clipEditor: root.clipEditor
                                        arrangerObject: region
                                        automationTrack: automationRegionLoader.automationTrack
                                        height: automationTrack.height
                                        x: regionX
                                        width: regionWidth
                                    }

                                }

                            }

                        }

                    }

                }

            }

        }

    }

}
