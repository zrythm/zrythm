// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  required property AudioEngine audioEngine
  required property Channel channel
  required property PluginImporter pluginImporter
  required property Track track
  required property Tracklist tracklist
  required property UndoStack undoStack

  implicitWidth: 48

  Rectangle {
    Layout.fillWidth: true
    Layout.minimumHeight: 10
    color: root.track.color
  }

  Label {
    Layout.fillWidth: true
    text: root.track.name
  }

  PluginSlotList {
    Layout.fillWidth: true
    Layout.minimumHeight: contentHeight
    pluginGroup: root.channel.midiFx
    pluginImporter: root.pluginImporter
    track: root.track

    header: Label {
      text: "MIDI FX"
    }
  }

  PluginSlotList {
    Layout.fillWidth: true
    Layout.minimumHeight: contentHeight
    pluginGroup: root.channel.inserts
    pluginImporter: root.pluginImporter
    track: root.track

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
