// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
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
  required property Track track
  required property var trackModelIndex
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  implicitWidth: 48

  SelectionTracker {
    id: selectionTracker

    modelIndex: root.trackModelIndex
    selectionModel: root.trackSelectionModel
  }

  Rectangle {
    Layout.fillHeight: true
    Layout.fillWidth: true
    color: {
      let c = root.palette.window;
      if (root.hovered)
        c = Style.getColorBlendedTowardsContrast(c);

      if (selectionTracker.isSelected || bgTapHandler.pressed)
        c = Qt.alpha(root.palette.highlight, 0.15);

      return c;
    }

    Behavior on color {
      animation: Style.propertyAnimation
    }

    HoverHandler {
      onHoveredChanged: root.hovered = hovered
    }

    TapHandler {
      id: bgTapHandler

      acceptedButtons: Qt.LeftButton | Qt.RightButton
      acceptedModifiers: Qt.NoModifier

      onTapped: (eventPoint, button) => {
        root.trackSelectionModel.selectSingleTrack(root.trackModelIndex);
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ControlModifier

      onTapped: (eventPoint, button) => {
        root.trackSelectionModel.select(root.trackModelIndex, ItemSelectionModel.Toggle);
        root.trackSelectionModel.setCurrentIndex(root.trackModelIndex, ItemSelectionModel.NoUpdate);
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        const currentIdx = root.trackSelectionModel.currentIndex;
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, root.trackModelIndex.row);
          const bottom = Math.max(currentIdx.row, root.trackModelIndex.row);
          const sel = QmlUtils.createRangeSelection(root.trackSelectionModel.model, top, bottom);
          root.trackSelectionModel.select(sel, ItemSelectionModel.ClearAndSelect);
        } else {
          root.trackSelectionModel.setCurrentIndex(root.trackModelIndex, ItemSelectionModel.Select);
        }
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ControlModifier | Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        const currentIdx = root.trackSelectionModel.currentIndex;
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, root.trackModelIndex.row);
          const bottom = Math.max(currentIdx.row, root.trackModelIndex.row);
          const sel = QmlUtils.createRangeSelection(root.trackSelectionModel.model, top, bottom);
          root.trackSelectionModel.select(sel, ItemSelectionModel.Select);
        } else {
          root.trackSelectionModel.setCurrentIndex(root.trackModelIndex, ItemSelectionModel.Select);
        }
      }
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 0

      Rectangle {
        Layout.fillWidth: true
        Layout.minimumHeight: 10
        color: root.track.color
      }

      Label {
        Layout.fillWidth: true
        elide: Text.ElideRight
        font: selectionTracker.isSelected ? Style.buttonTextFont : Style.normalTextFont
        text: root.track.name
      }

      PluginSlotList {
        Layout.fillWidth: true
        Layout.minimumHeight: contentHeight
        pluginGroup: root.channel.midiFx
        pluginImporter: root.pluginImporter
        pluginOperator: root.pluginOperator
        track: root.track
        trackSelectionModel: root.trackSelectionModel

        header: Label {
          text: "MIDI FX"
        }
      }

      PluginSlotList {
        Layout.fillWidth: true
        Layout.minimumHeight: contentHeight
        pluginGroup: root.channel.inserts
        pluginImporter: root.pluginImporter
        pluginOperator: root.pluginOperator
        track: root.track
        trackSelectionModel: root.trackSelectionModel

        header: Label {
          text: "Inserts"
        }
      }

      BalanceControl {
        Layout.fillHeight: false
        Layout.fillWidth: true
        balanceParameter: root.channel.fader.balance
        undoStack: root.undoStack
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
          undoStack: root.undoStack
        }

        TrackMeters {
          Layout.fillHeight: true
          Layout.fillWidth: false
          audioEngine: root.audioEngine
          channel: root.channel
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
