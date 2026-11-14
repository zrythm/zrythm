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

  // Shared mixer section state, driven by the strips' section headers
  // and content heights
  QtObject {
    id: mixerSectionMetrics

    property bool insertsExpanded: true
    property bool midiFxExpanded: true
    property bool sendsExpanded: true

    property bool anyMidiFx: false
    property real insertsBodyHeight: 0
    property real midiFxBodyHeight: 0
    property real sendsBodyHeight: 0

    signal dirty

    function markDirty(): void {
      dirty();
    }
  }

  Component.onCompleted: root.recomputeSectionMetrics()

  Connections {
    target: mixerSectionMetrics

    function onDirty(): void {
      root.recomputeSectionMetrics();
    }
  }

  function recomputeSectionMetrics(): void {
    let anyFx = false;
    let fx = 0;
    let ins = 0;
    let snd = 0;
    for (let i = 0; i < channelRepeater.count; i++) {
      const item = channelRepeater.itemAt(i);
      if (item === null)
        continue;
      anyFx = anyFx || item.supportsMidiFx;
      fx = Math.max(fx, item.midiFxContentHeight);
      ins = Math.max(ins, item.insertsContentHeight);
      snd = Math.max(snd, item.sendsContentHeight);
    }
    const master = masterLoader.item;
    if (master !== null) {
      anyFx = anyFx || master.supportsMidiFx;
      fx = Math.max(fx, master.midiFxContentHeight);
      ins = Math.max(ins, master.insertsContentHeight);
      snd = Math.max(snd, master.sendsContentHeight);
    }
    mixerSectionMetrics.anyMidiFx = anyFx;
    mixerSectionMetrics.midiFxBodyHeight = fx;
    mixerSectionMetrics.insertsBodyHeight = ins;
    mixerSectionMetrics.sendsBodyHeight = snd;
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
          id: channelRepeater

          delegate: ChannelView {
            id: stripDelegate

            audioEngine: root.audioEngine
            channel: stripDelegate.track.channel
            height: channelsRow.height
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

          onItemAdded: (index, item) => root.recomputeSectionMetrics()
          onItemRemoved: (index, item) => root.recomputeSectionMetrics()

          model: SortFilterProxyModel {
            id: mixerProxyModel

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

      onItemChanged: root.recomputeSectionMetrics()
      onLoaded: root.recomputeSectionMetrics()
    }
  }
}
