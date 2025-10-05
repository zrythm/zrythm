// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Zrythm

import Qt.labs.synchronizer

// Note: ColumnLayout wasn't working correctly so we're using Item for now
Item {
  id: root

  required property ClipLauncher clipLauncher
  required property ClipPlaybackService clipPlaybackService
  readonly property EditorSettings editorSettings: timeline.editorSettings

  // Synchronize horizontal scrolling
  property real horizontalScrollPosition: 0
  required property ArrangerObjectCreator objectCreator
  readonly property url playIconSource: ResourceManager.getIconUrl("gnome-icon-library", "media-playback-start-symbolic.svg")
  property int rulerHeight: 24
  property int sceneWidth: 80
  readonly property url stopIconSource: ResourceManager.getIconUrl("gnome-icon-library", "media-playback-stop-symbolic.svg")
  readonly property url switchToTimelineIconSource: ResourceManager.getIconUrl("gnome-icon-library", "ruler-angled-symbolic.svg")
  required property var timeline
  required property TrackCollection trackCollection

  TrackFilterProxyModel {
    id: pinnedTrackModel

    sourceModel: root.trackCollection

    Component.onCompleted: {
      addVisibilityFilter(true);
      addPinnedFilter(true);
    }
  }

  TrackFilterProxyModel {
    id: unpinnedTrackModel

    sourceModel: root.trackCollection

    Component.onCompleted: {
      addVisibilityFilter(true);
      addPinnedFilter(false);
    }
  }

  RowLayout {
    id: header

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: root.rulerHeight

    ToolButton {
      icon.source: root.stopIconSource

      onClicked: root.clipPlaybackService.stopAllClips()

      ToolTip {
        text: qsTr("Stop Scenes")
      }
    }

    ScrollView {
      id: sceneHeadersScrollView

      Layout.fillWidth: true

      // Only allow horizontal scrolling
      ScrollBar.horizontal.policy: ScrollBar.AsNeeded
      ScrollBar.vertical.policy: ScrollBar.AlwaysOff

      RowLayout {
        id: sceneHeaderColumns

        Repeater {
          model: root.clipLauncher.scenes

          delegate: RowLayout {
            id: sceneHeaderRow

            required property int index
            required property Scene scene

            Layout.fillWidth: true
            Layout.preferredHeight: root.rulerHeight
            Layout.preferredWidth: root.sceneWidth

            ToolButton {
              id: playSceneButton

              icon.source: root.playIconSource

              onClicked: root.clipPlaybackService.launchScene(sceneHeaderRow.scene)

              ToolTip {
                text: qsTr("Play Scene")
              }
            }

            Label {
              id: sceneHeaderLabel

              text: "Scene " + (sceneHeaderRow.index + 1)
            }
          }
        }
      }

      Synchronizer {
        sourceObject: root
        sourceProperty: "horizontalScrollPosition"
        targetObject: sceneHeadersScrollView.contentItem
        targetProperty: "contentX"
      }
    }

    ToolButton {
      icon.source: root.switchToTimelineIconSource

      ToolTip {
        text: qsTr("Switch Playback to Timeline")
      }
    }
  }

  RowsForTrackModel {
    id: pinnedTracksSection

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: header.bottom
    anchors.topMargin: 2
    // height: childrenRect.height
    model: pinnedTrackModel
  }

  ScrollView {
    id: unpinnedTracksScrollView

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: pinnedTracksSection.bottom
    anchors.topMargin: 2

    Synchronizer {
      sourceObject: root.editorSettings
      sourceProperty: "y"
      targetObject: unpinnedTracksScrollView.contentItem
      targetProperty: "contentY"
    }

    RowsForTrackModel {
      id: unpinnedTracksSection

      model: unpinnedTrackModel
      width: unpinnedTracksScrollView.width
    }
  }

  component RowsForTrackModel: RowLayout {
    id: rowsForTrackModel

    required property TrackFilterProxyModel model

    TrackStopButtonListView {
      Layout.preferredHeight: contentHeight
      Layout.preferredWidth: contentWidth
      model: rowsForTrackModel.model
    }

    ScrollView {
      id: sceneColumnsHorizontalScrollView

      Layout.fillWidth: true
      Layout.preferredHeight: contentHeight // disable vertical scroll

      ScrollBar.horizontal.policy: ScrollBar.AsNeeded
      ScrollBar.vertical.policy: ScrollBar.AlwaysOff

      RowLayout {
        id: sceneColumnsForTrackModel

        Repeater {
          model: root.clipLauncher.scenes

          delegate: TrackSceneClipSlotListView {
            required property int index

            Layout.preferredHeight: contentHeight
            Layout.preferredWidth: root.sceneWidth
            model: rowsForTrackModel.model
          }
        }
      }

      Synchronizer {
        sourceObject: root
        sourceProperty: "horizontalScrollPosition"
        targetObject: sceneColumnsHorizontalScrollView.contentItem
        targetProperty: "contentX"
      }
    }

    TrackSwitchPlaybackButtonListView {
      Layout.preferredHeight: contentHeight
      Layout.preferredWidth: contentWidth
      Layout.rightMargin: 4
      model: rowsForTrackModel.model
    }
  }
  component TrackSceneClipSlotListView: ListView {
    id: clipSlotsListView

    required property Scene scene

    boundsBehavior: Flickable.StopAtBounds

    delegate: ClipSlotView {
      id: trackClipSlotForScene

      clipPlaybackService: root.clipPlaybackService
      clipSlot: clipSlotsListView.scene.clipSlots.clipSlotForTrack(track)
      height: track.fullVisibleHeight
      objectCreator: root.objectCreator
      scene: clipSlotsListView.scene
      width: root.sceneWidth
    }
  }
  component TrackStopButtonListView: ListView {
    boundsBehavior: Flickable.StopAtBounds
    implicitWidth: 20

    delegate: ToolButton {
      id: trackStopButton

      required property Track track

      icon.source: root.stopIconSource
      implicitHeight: track.fullVisibleHeight

      ToolTip {
        text: "Stop " + trackStopButton.track.name
      }
    }
  }
  component TrackSwitchPlaybackButtonListView: ListView {
    boundsBehavior: Flickable.StopAtBounds
    implicitWidth: 20

    delegate: ToolButton {
      id: trackSwitchPlaybackButton

      required property Track track

      enabled: track.clipLauncherMode
      icon.source: root.switchToTimelineIconSource
      implicitHeight: track.fullVisibleHeight

      onClicked: root.clipPlaybackService.setTrackToTimelineMode(track)

      ToolTip {
        text: "Switch Playback to Timeline"
      }
    }
  }
}
