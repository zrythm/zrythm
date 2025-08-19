// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

RowLayout {
  id: root

  required property Channel channel
  readonly property real currentAmplitude: audioMetersLoader.active ? audioMetersLoader.item.currentAmplitude : midiMetersLoader.item.currentAmplitude
  readonly property real currentPeak: audioMetersLoader.active ? audioMetersLoader.item.currentPeak : midiMetersLoader.item.currentPeak

  Loader {
    id: audioMetersLoader

    Layout.fillHeight: true
    Layout.fillWidth: false
    active: root.channel && root.channel.leftAudioOut
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
        Layout.preferredWidth: width
        port: root.channel.leftAudioOut
      }

      Meter {
        id: audioRMeter

        Layout.fillHeight: true
        Layout.preferredWidth: width
        port: root.channel.rightAudioOut
      }
    }
  }

  Loader {
    id: midiMetersLoader

    Layout.fillHeight: true
    Layout.fillWidth: false
    Layout.preferredWidth: 4
    active: root.channel && root.channel.midiOut
    visible: active

    sourceComponent: Meter {
      id: midiMeter

      property real currentAmplitude: midiMeter.meterProcessor.currentAmplitude
      property real currentPeak: midiMeter.meterProcessor.peakAmplitude

      anchors.fill: parent
      port: root.channel.midiOut
    }
  }
}
