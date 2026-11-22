// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

Item {
  id: root

  readonly property bool _isMasterTrack: root.track && root.track.type === Track.Master
  required property AudioEngine audioEngine
  required property Channel channel
  readonly property real currentAmplitude: root.channel ? (audioMetersLoader.active ? audioMetersLoader.item.currentAmplitude : (midiMetersLoader.active ? midiMetersLoader.item.currentAmplitude : 0)) : 0
  readonly property real currentPeak: root.channel ? (audioMetersLoader.active ? audioMetersLoader.item.currentPeak : (midiMetersLoader.active ? midiMetersLoader.item.currentPeak : 0)) : 0
  // Maximum peak level observed since the last resetPeakHold(); numeric
  // readouts show this instead of the decaying bar peak
  readonly property real currentPeakHold: root.channel ? (audioMetersLoader.active ? audioMetersLoader.item.currentPeakHold : (midiMetersLoader.active ? midiMetersLoader.item.currentPeakHold : 0)) : 0
  // Whether the channel carries audio meters (false for MIDI-only tracks)
  readonly property bool hasAudio: audioMetersLoader.active
  implicitHeight: meterLayout.implicitHeight
  implicitWidth: meterLayout.implicitWidth
  required property PortObservationManager portObservationManager
  // Peak level as text for readouts next to the meters
  readonly property string peakDbText: formatDbfsText(currentPeakHold)
  // True while the held peak level exceeds full scale
  readonly property bool peakOver: QmlUtils.amplitudeToDbfs(currentPeakHold) > 0
  // Draws dB tick labels along the audio meters (mixer strips)
  property bool showScale: false
  // Set false when another component presents MIDI event activity, so the
  // generic MIDI meter is not duplicated.
  property bool showMidiMeter: true
  required property Track track

  function formatDbfsText(amplitude: real): string {
    const dbfs = QmlUtils.amplitudeToDbfs(amplitude);
    if (dbfs < -98)
      return "-\u221e";
    return Number(dbfs).toFixed(dbfs < -10 ? 0 : 1);
  }

  function resetPeakHold() {
    if (audioMetersLoader.active)
      audioMetersLoader.item.resetPeakHold();
    if (midiMetersLoader.active)
      midiMetersLoader.item.resetPeakHold();
  }

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
        property real currentPeakHold: Math.max(audioLMeter.meterProcessor.peakHoldAmplitude, audioRMeter.meterProcessor.peakHoldAmplitude)

        // Where a dB level sits along the bars, through the same fader
        // transform the bars themselves draw with
        function faderPosForDb(db: real): real {
          return audioLMeter.meterProcessor.toFader(Math.pow(10, db / 20));
        }

        function resetPeakHold() {
          audioLMeter.meterProcessor.resetPeakHold();
          audioRMeter.meterProcessor.resetPeakHold();
        }

        anchors.fill: parent
        spacing: 0

        Meter {
          id: audioLMeter

          // The master strip meters with K-system ballistics and the
          // segmented K-system look; other channels draw continuous bars
          readonly property bool isMaster: root._isMasterTrack

          Layout.fillHeight: true
          Layout.preferredWidth: 4
          algorithm: isMaster ? MeterProcessor.K : MeterProcessor.DigitalPeak
          audioEngine: root.audioEngine
          channel: 0
          k20Colors: isMaster
          port: root.channel.audioOutPort
          portObservationManager: root.portObservationManager
          segmentCount: isMaster ? 22 : 0
        }

        Meter {
          id: audioRMeter

          readonly property bool isMaster: root._isMasterTrack

          Layout.fillHeight: true
          Layout.preferredWidth: 4
          algorithm: isMaster ? MeterProcessor.K : MeterProcessor.DigitalPeak
          audioEngine: root.audioEngine
          channel: 1
          k20Colors: isMaster
          port: root.channel.audioOutPort
          portObservationManager: root.portObservationManager
          segmentCount: isMaster ? 22 : 0
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
        property real currentPeakHold: midiMeter.meterProcessor.peakHoldAmplitude

        function resetPeakHold() {
          midiMeter.meterProcessor.resetPeakHold();
        }

        anchors.fill: parent
        audioEngine: root.audioEngine
        port: root.channel.midiOut
        portObservationManager: root.portObservationManager
      }
    }

    // dB scale along the audio meters
    Item {
      Layout.fillHeight: true
      Layout.preferredWidth: 16
      visible: root.showScale && audioMetersLoader.active

      Repeater {
        model: [0, -6, -12, -24, -48]

        Label {
          required property int modelData

          anchors.right: parent.right
          color: modelData === 0 ? Qt.alpha(palette.windowText, 0.8) : Qt.alpha(palette.windowText, 0.4)
          font: ZrythmTheme.xxSmallTextFont
          text: modelData === 0 ? "0" : String(modelData)
          y: Math.min(Math.max(parent.height * (1 - (audioMetersLoader.item ? audioMetersLoader.item.faderPosForDb(modelData) : 0)) - height / 2, 0), parent.height - height)
        }
      }
    }
  }
}
