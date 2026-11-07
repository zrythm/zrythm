// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

Item {
  id: root

  readonly property bool _isMasterTrack: root.track && root.track.type === Track.Master
  required property AudioEngine audioEngine
  required property Channel channel
  readonly property real currentAmplitude: root.channel ? (audioMetersLoader.active ? audioMetersLoader.item.currentAmplitude : (midiMetersLoader.active ? midiMetersLoader.item.currentAmplitude : 0)) : 0
  readonly property real currentPeak: root.channel ? (audioMetersLoader.active ? audioMetersLoader.item.currentPeak : (midiMetersLoader.active ? midiMetersLoader.item.currentPeak : 0)) : 0
  implicitHeight: meterLayout.implicitHeight
  implicitWidth: meterLayout.implicitWidth
  required property PortObservationManager portObservationManager
  // Set false when another component presents MIDI event activity, so the
  // generic MIDI meter is not duplicated.
  property bool showMidiMeter: true
  required property Track track

  RowLayout {
    id: meterLayout

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.top: parent.top
    spacing: 0
    width: implicitWidth

    Loader {
      id: audioMetersLoader

      Layout.fillHeight: true
      Layout.fillWidth: false
      active: root.channel && root.channel.audioOutPort
      visible: active

      sourceComponent: RowLayout {
        id: meters

        property real currentAmplitude: Math.max(audioLMeter.meterProcessor.currentAmplitude, audioRMeter.meterProcessor.currentAmplitude)
        property real currentPeak: Math.max(audioLMeter.meterProcessor.peakAmplitude, audioRMeter.meterProcessor.peakAmplitude)

        anchors.fill: parent
        spacing: 0

        Meter {
          id: audioLMeter

          Layout.fillHeight: true
          algorithm: root._isMasterTrack ? MeterProcessor.K : MeterProcessor.DigitalPeak
          audioEngine: root.audioEngine
          channel: 0
          port: root.channel.audioOutPort
          portObservationManager: root.portObservationManager
        }

        Meter {
          id: audioRMeter

          Layout.fillHeight: true
          algorithm: root._isMasterTrack ? MeterProcessor.K : MeterProcessor.DigitalPeak
          audioEngine: root.audioEngine
          channel: 1
          port: root.channel.audioOutPort
          portObservationManager: root.portObservationManager
        }
      }
    }

    Loader {
      id: midiMetersLoader

      Layout.fillHeight: true
      Layout.fillWidth: false
      Layout.preferredWidth: 4
      active: root.channel && root.channel.midiOut && root.showMidiMeter
      visible: active

      sourceComponent: Meter {
        id: midiMeter

        property real currentAmplitude: midiMeter.meterProcessor.currentAmplitude
        property real currentPeak: midiMeter.meterProcessor.peakAmplitude

        anchors.fill: parent
        audioEngine: root.audioEngine
        port: root.channel.midiOut
        portObservationManager: root.portObservationManager
      }
    }
  }
}
