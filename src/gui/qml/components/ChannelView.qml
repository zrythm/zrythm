// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  required property Channel channel
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

  ListView {
    Layout.fillHeight: true
    Layout.fillWidth: true
    Layout.preferredHeight: 100
    model: root.channel.midiFx

    delegate: PluginSlotView {
      track: root.track
    }
    header: Label {
      text: "MIDI FX"
    }
  }

  ListView {
    Layout.fillHeight: true
    Layout.fillWidth: true
    Layout.preferredHeight: 100
    model: root.channel.inserts

    delegate: PluginSlotView {
      track: root.track
    }
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
      channel: root.channel
    }
  }

  ComboBox {
    Layout.fillWidth: true
    textRole: "trackName"
    valueRole: "track"

    model: TrackFilterProxyModel {
      sourceModel: root.tracklist.collection
    }

    Component.onCompleted: currentIndex = indexOfValue(root.tracklist.trackRouting.getOutputTrack(root.track))

    // TODO: use an action class like TrackRoutingOperator
    onActivated: root.tracklist.trackRouting.setOutputTrack(root.track, currentValue)
  }
}
