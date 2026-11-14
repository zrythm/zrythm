// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import Zrythm

// Mixer surface: the channel strips laid out horizontally at a fixed
// width in a scrollable row, with the master strip pinned at the
// trailing edge. Section fold state and section body heights are shared
// by every strip: folding a level folds it in all channels, and each
// section takes the tallest content among strips so sections line up.
Item {
  id: root

  required property AudioEngine audioEngine
  required property OperationDispatcher operationDispatcher
  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  required property PortObservationManager portObservationManager
  required property ProcessorParameterOperator parameterOperator
  required property Project project
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  signal pluginClicked(Plugin plugin)

  readonly property int stripWidth: 112

  // Shared section state: strips report their section content to it and
  // bind their section bodies to its uniform heights
  MixerSectionMetrics {
    id: mixerSectionMetrics
  }

  RowLayout {
    anchors.fill: parent
    spacing: 2

    Flickable {
      id: channelsScroll

      Layout.fillHeight: true
      Layout.fillWidth: true
      boundsBehavior: Flickable.StopAtBounds
      clip: true
      // Trailing room so the strips can be scrolled past the last one
      contentHeight: height
      contentWidth: channelsRow.width + width / 2

      ScrollBar.horizontal: ScrollBar {
      }

      Row {
        id: channelsRow

        height: channelsScroll.height
        spacing: 2

        Repeater {
          delegate: ChannelView {
            id: stripDelegate

            audioEngine: root.audioEngine
            channel: stripDelegate.track.channel
            height: channelsRow.height
            operationDispatcher: root.operationDispatcher
            parameterOperator: root.parameterOperator
            pluginImporter: root.pluginImporter
            pluginOperator: root.pluginOperator
            portObservationManager: root.portObservationManager
            project: root.project
            sectionMetrics: mixerSectionMetrics
            trackSelectionModel: root.trackSelectionModel
            tracklist: root.tracklist
            undoStack: root.undoStack
            width: root.stripWidth

            onPluginClicked: function(plugin: Plugin) {
              root.pluginClicked(plugin);
            }
          }

          model: SortFilterProxyModel {
            sourceModel: root.tracklist.collection

            filters: [
              FunctionFilter {
                function filter(track: Track): bool {
                  return track.type !== Track.Master && root.tracklist.shouldBeVisible(track) && root.tracklist.hasChannel(track);
                }
              }
            ]
          }
        }
      }
    }

    Loader {
      id: masterLoader

      Layout.fillHeight: true
      Layout.preferredWidth: root.stripWidth
      active: root.tracklist.singletonTracks.masterTrack !== null

      sourceComponent: ChannelView {
        id: masterStrip

        audioEngine: root.audioEngine
        channel: masterStrip.track.channel
        operationDispatcher: root.operationDispatcher
        parameterOperator: root.parameterOperator
        pluginImporter: root.pluginImporter
        pluginOperator: root.pluginOperator
        portObservationManager: root.portObservationManager
        project: root.project
        sectionMetrics: mixerSectionMetrics
        track: root.tracklist.singletonTracks.masterTrack
        trackSelectionModel: root.trackSelectionModel
        tracklist: root.tracklist
        undoStack: root.undoStack

        onPluginClicked: function(plugin: Plugin) {
          root.pluginClicked(plugin);
        }
      }
    }
  }
}
