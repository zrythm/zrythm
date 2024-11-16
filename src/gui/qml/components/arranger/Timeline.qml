// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Arranger {
    id: root

    required property bool pinned
    required property var timeline
    required property var tracklist

    editorSettings: timeline
    enableYScroll: !pinned
    scrollView.ScrollBar.horizontal.policy: pinned ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded

    content: ListView {
        id: tracksListView

        anchors.fill: parent

        model: TrackFilterProxyModel {
            sourceModel: tracklist
            Component.onCompleted: {
                addVisibilityFilter(true);
                addPinnedFilter(root.pinned);
            }
        }

        delegate: ItemDelegate {
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

                            model: trackDelegate.track

                            delegate: Loader {
                                id: scaleLoader

                                required property var scaleObject
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

                            model: trackDelegate.track

                            delegate: Loader {
                                id: markerLoader

                                required property var marker
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
                                    required property var region
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

                        AudioRegion {
                            ruler: root.ruler
                            track: trackDelegate.track
                            arrangerObject: region
                            lane: trackLane
                            height: trackLane.height
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
