// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
  id: root

  required property var project

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
          trackFactory: project.trackFactory
          tracklist: project.tracklist
        }

        Tracklist {
          id: pinnedTracklist

          Layout.fillWidth: true
          Layout.preferredHeight: contentHeight
          pinned: true
          tracklist: project.tracklist
        }

        Tracklist {
          id: unpinnedTracklist

          Layout.fillWidth: true
          Layout.maximumHeight: unpinnedTimelineArranger.height
          Layout.minimumHeight: unpinnedTimelineArranger.height
          pinned: false
          tracklist: project.tracklist
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
          ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
          ScrollBar.vertical.policy: ScrollBar.AlwaysOff
          height: ruler.height

          Ruler {
            id: ruler

            editorSettings: project.timeline.editorSettings
            tempoMap: project.tempoMap
            transport: project.transport
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

        Timeline {
          id: pinnedTimelineArranger

          Layout.fillWidth: true
          Layout.maximumHeight: pinnedTracklist.height
          Layout.minimumHeight: pinnedTracklist.height
          clipEditor: project.clipEditor
          objectFactory: project.arrangerObjectFactory
          pinned: true
          ruler: ruler
          tempoMap: project.tempoMap
          timeline: project.timeline
          tool: project.tool
          tracklist: project.tracklist
          transport: project.transport
        }

        Timeline {
          id: unpinnedTimelineArranger

          Layout.fillHeight: true
          Layout.fillWidth: true
          clipEditor: project.clipEditor
          objectFactory: project.arrangerObjectFactory
          pinned: false
          ruler: ruler
          tempoMap: project.tempoMap
          timeline: project.timeline
          tool: project.tool
          tracklist: project.tracklist
          transport: project.transport
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
