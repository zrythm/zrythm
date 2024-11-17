// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
    id: root

    required property var project

    spacing: 0

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: centerTabBar.currentIndex

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            ColumnLayout {
                id: leftPane

                spacing: 1
                SplitView.minimumWidth: 120
                SplitView.preferredWidth: 200

                TracklistHeader {
                    id: tracklistHeader

                    Layout.fillWidth: true
                    Layout.minimumHeight: ruler.height
                    Layout.maximumHeight: ruler.height
                    tracklist: project.tracklist
                }

                Tracklist {
                    id: pinnedTracklist

                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    tracklist: project.tracklist
                    pinned: true
                }

                Tracklist {
                    id: unpinnedTracklist

                    tracklist: project.tracklist
                    Layout.fillWidth: true
                    Layout.minimumHeight: unpinnedTimelineArranger.height
                    Layout.maximumHeight: unpinnedTimelineArranger.height
                    pinned: false
                }

            }

            ColumnLayout {
                id: rightPane

                spacing: 1
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumWidth: 200

                ScrollView {
                    id: rulerScrollView

                    Layout.fillWidth: true
                    height: ruler.height
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff

                    Ruler {
                        id: ruler

                        editorSettings: project.timeline
                        transport: project.transport
                    }

                    Binding {
                        target: rulerScrollView.contentItem
                        property: "contentX"
                        value: root.project.timeline.x
                    }

                    Connections {
                        function onContentXChanged() {
                            root.project.timeline.x = rulerScrollView.contentItem.contentX;
                        }

                        target: rulerScrollView.contentItem
                    }

                }

                Timeline {
                    id: pinnedTimelineArranger

                    pinned: true
                    timeline: project.timeline
                    tracklist: project.tracklist
                    ruler: ruler
                    selections: project.timelineSelections
                    tool: project.tool
                    Layout.fillWidth: true
                    Layout.minimumHeight: pinnedTracklist.height
                    Layout.maximumHeight: pinnedTracklist.height
                }

                Timeline {
                    id: unpinnedTimelineArranger

                    pinned: false
                    timeline: project.timeline
                    tracklist: project.tracklist
                    ruler: ruler
                    selections: project.timelineSelections
                    tool: project.tool
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

            }

        }

        Item {
            id: portConnectionsTab

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Item {
            id: midiCcBindingsTab

            Layout.fillWidth: true
            Layout.fillHeight: true
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
