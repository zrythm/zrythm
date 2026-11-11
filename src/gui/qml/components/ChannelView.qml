// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  required property AudioEngine audioEngine
  required property Channel channel
  property bool hovered: false
  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  required property PortObservationManager portObservationManager
  required property ProcessorParameterOperator parameterOperator
  required property Project project
  required property Track track
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  signal pluginClicked(Plugin plugin)

  // Returns a fresh source-model index for this delegate's track. Must be
  // called at time of use - QModelIndex values must not be cached in QML
  // because they are frozen snapshots that go stale when model rows change.
  function trackModelIndex(): var {
    return tracklist.collection.indexForTrack(track);
  }

  implicitWidth: 48

  SelectionTracker {
    id: selectionTracker

    modelIndexProvider: root.trackModelIndex
    selectionModel: root.trackSelectionModel
  }

  Rectangle {
    Layout.fillHeight: true
    Layout.fillWidth: true
    color: ZrythmTheme.surfaceColor (
      root.palette.window, root.palette.windowText,
      selectionTracker.isSelected || bgTapHandler.pressed, root.hovered)

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }

    HoverHandler {
      onHoveredChanged: root.hovered = hovered
    }

    TapHandler {
      id: bgTapHandler

      acceptedButtons: Qt.LeftButton | Qt.RightButton
      acceptedModifiers: Qt.NoModifier

      onTapped: (eventPoint, button) => {
        root.trackSelectionModel.selectSingleTrack(root.trackModelIndex());
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ControlModifier

      onTapped: (eventPoint, button) => {
        root.trackSelectionModel.select(root.trackModelIndex(), ItemSelectionModel.Toggle);
        root.trackSelectionModel.setCurrentIndex(root.trackModelIndex(), ItemSelectionModel.NoUpdate);
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        const currentIdx = root.trackSelectionModel.currentIndex;
        const thisIdx = root.trackModelIndex();
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, thisIdx.row);
          const bottom = Math.max(currentIdx.row, thisIdx.row);
          const sel = QmlUtils.createRangeSelection(root.trackSelectionModel.model, top, bottom);
          root.trackSelectionModel.select(sel, ItemSelectionModel.ClearAndSelect);
        } else {
          root.trackSelectionModel.setCurrentIndex(thisIdx, ItemSelectionModel.Select);
        }
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ControlModifier | Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        const currentIdx = root.trackSelectionModel.currentIndex;
        const thisIdx = root.trackModelIndex();
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, thisIdx.row);
          const bottom = Math.max(currentIdx.row, thisIdx.row);
          const sel = QmlUtils.createRangeSelection(root.trackSelectionModel.model, top, bottom);
          root.trackSelectionModel.select(sel, ItemSelectionModel.Select);
        } else {
          root.trackSelectionModel.setCurrentIndex(thisIdx, ItemSelectionModel.Select);
        }
      }
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 0

      Rectangle {
        Layout.fillWidth: true
        Layout.minimumHeight: 4
        color: TrackColors.effectiveColor(root.track, ZrythmTheme.darkMode)
      }

      Label {
        Layout.fillWidth: true
        elide: Text.ElideRight
        font: selectionTracker.isSelected ? ZrythmTheme.buttonTextFont : ZrythmTheme.normalTextFont
        text: root.track.name
      }

      ExpanderBox {
        Layout.fillWidth: true
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "audio-insert.svg")
        title: qsTr("MIDI FX")

        frameContentItem: PluginSlotList {
          pluginGroup: root.channel.midiFx
          pluginImporter: root.pluginImporter
          pluginOperator: root.pluginOperator
          track: root.track
          trackSelectionModel: root.trackSelectionModel

          onPluginClicked: function (plugin: Plugin) {
            root.pluginClicked(plugin);
          }
        }
      }

      ExpanderBox {
        Layout.fillWidth: true
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "audio-insert.svg")
        title: qsTr("Inserts")

        frameContentItem: PluginSlotList {
          pluginGroup: root.channel.inserts
          pluginImporter: root.pluginImporter
          pluginOperator: root.pluginOperator
          track: root.track
          trackSelectionModel: root.trackSelectionModel

          onPluginClicked: function (plugin: Plugin) {
            root.pluginClicked(plugin);
          }
        }
      }

      ChannelSendsPanel {
        Layout.fillWidth: true
        expanded: false
        headerIconName: ""
        visible: root.track.type !== Track.Master

        channelSendRouting: root.tracklist.channelSendRouting
        parameterOperator: root.parameterOperator
        project: root.project
        sends: root.channel.sends
        track: root.track
        undoStack: root.undoStack
      }

      BalanceControl {
        Layout.fillHeight: false
        Layout.fillWidth: true
        balanceParameter: root.channel.fader.balance
        parameterOperator: root.parameterOperator
      }

      RowLayout {
        Layout.fillHeight: true
        Layout.fillWidth: false

        FaderButtons {
          Layout.fillHeight: true
          Layout.fillWidth: false
          fader: root.channel.fader
          track: root.track
        }

        FaderControl {
          Layout.fillHeight: true
          Layout.fillWidth: false
          faderGain: root.channel.fader.gain
          parameterOperator: root.parameterOperator
        }

        TrackMeters {
          Layout.fillHeight: true
          Layout.fillWidth: false
          audioEngine: root.audioEngine
          channel: root.channel
          portObservationManager: root.portObservationManager
          track: root.track
        }
      }

      TrackRouteControl {
        track: root.track
        tracklist: root.tracklist
        undoStack: root.undoStack
      }
    }
  }
}
