// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Qt.labs.synchronizer
import Zrythm

ScrollView {
  id: root

  required property AudioEngine audioEngine
  required property DeviceManager deviceManager
  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  required property ProjectSession session
  required property Track track
  required property TrackSelectionModel trackSelectionModel
  required property UndoStack undoStack

  signal pluginClicked(Plugin plugin)

  ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
  contentWidth: availableWidth

  ColumnLayout {
    id: content

    width: root.availableWidth

    TrackOperator {
      id: trackOperator

      track: root.track
      undoStack: root.undoStack
    }

    ExpanderBox {
      Layout.fillWidth: true
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "general-properties-symbolic.svg")
      title: qsTr("Track Properties")

      frameContentItem: ColumnLayout {
        spacing: 4

        Label {
          text: qsTr("Name")
        }

        EditableLabel {
          id: trackNameLabel

          Layout.fillWidth: true
          text: root.track.name

          onAccepted: function (newText) {
            trackOperator.rename(newText);
          }
        }

        Label {
          text: qsTr("Color")
        }

        Rectangle {
          id: colorRect

          Layout.fillWidth: true
          Layout.preferredHeight: 20
          color: root.track.color
          radius: 4

          MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor

            onClicked: colorDialog.open()
          }

          ColorDialog {
            id: colorDialog

            selectedColor: root.track.color
            title: qsTr("Choose Track Color")

            onAccepted: {
              trackOperator.setColor(selectedColor);
            }
          }
        }

        Label {
          text: qsTr("Notes")
        }

        ScrollView {
          Layout.fillWidth: true
          Layout.preferredHeight: Math.min(notesTextArea.implicitHeight + 16, 100)

          TextArea {
            id: notesTextArea

            text: root.track.comment
            wrapMode: TextEdit.Wrap

            onEditingFinished: {
              if (text !== root.track.comment) {
                trackOperator.setComment(text);
              }
            }

            Connections {
              function onCommentChanged(comment: string) {
                notesTextArea.text = comment;
              }

              target: root.track
            }
          }
        }

        Label {
          text: qsTr("Timebase")
          visible: root.track.type === Track.Audio
        }

        ComboBox {
          id: timebaseCombo

          Layout.fillWidth: true
          model: [qsTr("Musical"), qsTr("Absolute")]
          visible: root.track.type === Track.Audio

          Synchronizer on currentIndex {
            sourceObject: root.track.timebaseProvider
            sourceProperty: "effectiveTimebase"
          }

          onActivated: function (index: int): void {
            trackOperator.setTimebase(index);
          }

          Connections {
            function onTrackChanged() {
              timebaseCombo.currentIndex = root.track.timebaseProvider ? root.track.timebaseProvider.effectiveTimebase : 0;
            }

            target: root
          }
        }
      }
    }

    Loader {
      Layout.fillWidth: true
      active: root.track.type === Track.Audio
      visible: active

      sourceComponent: ExpanderBox {
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "source-pick-symbolic.svg")
        title: qsTr("Device Input")

        frameContentItem: ColumnLayout {
          spacing: 4

          Label {
            Layout.fillWidth: true
            text: qsTr("Audio Input")
          }

          ComboBox {
            id: inputSelector

            property AudioInputSelection inputSel: root.session.uiState.audioInputSelectionForTrack(root.track)

            function formatInput(info: audioInputInfo): string {
              if (info.stereo)
                return qsTr("Input %1-%2 (Stereo)").arg(info.firstChannel + 1).arg(info.firstChannel + 2);
              return qsTr("Input %1 (Mono)").arg(info.firstChannel + 1);
            }

            function updateCurrentIndex(): void {
              if (!inputSel || inputSel.deviceName === "") {
                currentIndex = 0;
                return;
              }
              for (let i = 1; i < count; ++i) {
                const info = model[i].inputInfo;
                if (info && info.deviceName === inputSel.deviceName && info.firstChannel === inputSel.firstChannel && info.stereo === inputSel.stereo) {
                  currentIndex = i;
                  return;
                }
              }
              currentIndex = 0;
            }

            Layout.fillWidth: true
            model: {
              const inputs = [
                {
                  "display": qsTr("None"),
                  "inputInfo": null
                }
              ];
              if (root.deviceManager) {
                for (const info of root.deviceManager.availableAudioInputs)
                  inputs.push({
                    "display": formatInput(info),
                    "inputInfo": info
                  });
              }
              return inputs;
            }
            textRole: "display"

            Component.onCompleted: updateCurrentIndex()
            onActivated: function (index: int): void {
              if (index === 0) {
                inputSelector.inputSel.deviceName = "";
                return;
              }
              const info = model[index].inputInfo;
              inputSelector.inputSel.deviceName = info.deviceName;
              inputSelector.inputSel.firstChannel = info.firstChannel;
              inputSelector.inputSel.stereo = info.stereo;
            }
            onInputSelChanged: updateCurrentIndex()
            onModelChanged: updateCurrentIndex()

            Connections {
              function onDeviceNameChanged() {
                inputSelector.updateCurrentIndex();
              }

              function onFirstChannelChanged() {
                inputSelector.updateCurrentIndex();
              }

              function onStereoChanged() {
                inputSelector.updateCurrentIndex();
              }

              target: inputSelector.inputSel
            }

            Connections {
              function onAvailableAudioInputsChanged() {
                inputSelector.updateCurrentIndex();
              }

              target: root.deviceManager
            }
          }

          Label {
            Layout.fillWidth: true
            text: qsTr("Monitoring")
            visible: monitorSelector.visible
          }

          ComboBox {
            id: monitorSelector

            property ProcessorParameter monitorParam: root.track.monitorParam

            Layout.fillWidth: true
            model: {
              if (!monitorParam)
                return [];
              const labels = [];
              for (let i = 0; i < monitorParam.range.enumCount(); ++i) {
                labels.push(monitorParam.range.enumLabel(i));
              }
              return labels;
            }
            visible: monitorParam !== null

            Component.onCompleted: {
              if (monitorParam) {
                currentIndex = monitorParam.range.enumIndex(monitorParam.baseValue);
              }
            }
            onActivated: function (index: int): void {
              if (monitorParam) {
                monitorParam.baseValue = monitorParam.range.normalizedEnumValue(index);
              }
            }
            onMonitorParamChanged: {
              if (monitorParam) {
                currentIndex = monitorParam.range.enumIndex(monitorParam.baseValue);
              }
            }

            Connections {
              function onBaseValueChanged() {
                if (monitorSelector.monitorParam) {
                  monitorSelector.currentIndex = monitorSelector.monitorParam.range.enumIndex(monitorSelector.monitorParam.baseValue);
                }
              }

              enabled: monitorSelector.monitorParam !== null
              target: monitorSelector.monitorParam
            }
          }
        }
      }
    }

    Loader {
      Layout.fillWidth: true
      active: root.track.type === Track.Instrument || root.track.type === Track.Midi
      visible: active

      sourceComponent: ExpanderBox {
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "input-keyboard-symbolic.svg")
        title: qsTr("MIDI Input")

        frameContentItem: ColumnLayout {
          spacing: 4

          Label {
            Layout.fillWidth: true
            text: qsTr("Device")
          }

          ComboBox {
            id: midiDeviceSelector

            property MidiInputSelection midiSel: root.session.uiState.midiInputSelectionForTrack(root.track)

            function updateCurrentIndex(): void {
              if (!midiSel || midiSel.deviceIdentifier === "") {
                currentIndex = 0;
                return;
              }
              for (let i = 1; i < count; ++i) {
                const info = model[i].midiInfo;
                if (info && info.identifier === midiSel.deviceIdentifier) {
                  currentIndex = i;
                  return;
                }
              }
              currentIndex = 0;
            }

            Layout.fillWidth: true
            model: {
              const devices = [
                {
                  "display": qsTr("None"),
                  "midiInfo": null
                }
              ];
              if (root.deviceManager) {
                for (const info of root.deviceManager.availableMidiInputs)
                  devices.push({
                    "display": info.deviceName,
                    "midiInfo": info
                  });
              }
              return devices;
            }
            textRole: "display"

            Component.onCompleted: updateCurrentIndex()
            onActivated: function (index: int): void {
              if (index === 0) {
                midiDeviceSelector.midiSel.deviceIdentifier = "";
                return;
              }
              const info = model[index].midiInfo;
              midiDeviceSelector.midiSel.deviceIdentifier = info.identifier;
            }
            onMidiSelChanged: updateCurrentIndex()
            onModelChanged: updateCurrentIndex()

            Connections {
              function onDeviceIdentifierChanged() {
                midiDeviceSelector.updateCurrentIndex();
              }

              target: midiDeviceSelector.midiSel
            }

            Connections {
              function onAvailableMidiInputsChanged() {
                midiDeviceSelector.updateCurrentIndex();
              }

              target: root.deviceManager
            }
          }

          Label {
            Layout.fillWidth: true
            text: qsTr("Channel")
          }

          ComboBox {
            id: midiChannelSelector

            property MidiInputSelection midiSel: root.session.uiState.midiInputSelectionForTrack(root.track)

            Layout.fillWidth: true
            model: {
              const channels = [qsTr("All")];
              for (let i = 1; i <= 16; ++i)
                channels.push(qsTr("Channel %1").arg(i));
              return channels;
            }

            Component.onCompleted: {
              if (midiSel)
                currentIndex = midiSel.midiChannel;
            }
            onActivated: function (index: int): void {
              if (midiSel)
                midiSel.midiChannel = index;
            }
            onMidiSelChanged: {
              if (midiSel)
                currentIndex = midiSel.midiChannel;
            }

            Connections {
              function onMidiChannelChanged() {
                if (midiChannelSelector.midiSel)
                  midiChannelSelector.currentIndex = midiChannelSelector.midiSel.midiChannel;
              }

              target: midiChannelSelector.midiSel
            }
          }
        }
      }
    }

    Loader {
      Layout.fillWidth: true
      active: (root.track.type === Track.Instrument || root.track.type === Track.Midi) && root.track.channel !== null && root.track.channel.midiFx !== null
      visible: active

      sourceComponent: PluginExpander {
        iconName: "audio-insert.svg"
        model: root.track.channel.midiFx
        title: "MIDI FX"
      }
    }

    Loader {
      Layout.fillWidth: true
      active: root.track.type === Track.Instrument
      visible: active

      sourceComponent: PluginExpander {
        iconName: "audio-insert.svg"
        model: root.track.channel.instruments
        title: qsTr("Instrument")
      }
    }

    Loader {
      Layout.fillWidth: true
      active: root.track.channel !== null && root.track.channel.inserts !== null
      visible: active

      sourceComponent: PluginExpander {
        iconName: "audio-insert.svg"
        model: root.track.channel.inserts
        title: qsTr("Inserts")
      }
    }

    Loader {
      Layout.fillWidth: true
      active: root.track.channel !== null
      visible: active

      sourceComponent: ExpanderBox {
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "fader.svg")
        title: "Fader"

        frameContentItem: ColumnLayout {
          BalanceControl {
            Layout.fillWidth: true
            balanceParameter: root.track.channel.fader.balance
            undoStack: root.undoStack
          }

          RowLayout {
            Layout.fillWidth: true

            FaderButtons {
              fader: root.track.channel.fader
              track: root.track
            }

            FaderControl {
              faderGain: root.track.channel.fader.gain
              undoStack: root.undoStack
            }

            TrackMeters {
              id: meters

              Layout.fillHeight: true
              Layout.fillWidth: false
              audioEngine: root.audioEngine
              channel: root.track.channel
              portObservationManager: root.session.project.portObservationManager
              track: root.track
            }

            TextMetrics {
              id: peakLabelMetrics

              font: peakLabel.font
              text: "Peak:\n-9.9 db"
            }

            Label {
              id: peakLabel

              readonly property real peak_in_dbfs: QmlUtils.amplitudeToDbfs(meters.currentPeak)

              Layout.minimumWidth: peakLabelMetrics.width
              text: {
                let txt_val = "Peak:\n";
                if (peak_in_dbfs < -98) {
                  txt_val += "-∞";
                } else {
                  let decimal_places = 1;
                  if (peak_in_dbfs < -10) {
                    decimal_places = 0;
                  }
                  txt_val += Number(peak_in_dbfs).toFixed(decimal_places);
                }
                return txt_val + " db";
              }

              Binding {
                property: "color"
                target: peakLabel
                value: ZrythmTheme.dangerColor
                when: peakLabel.peak_in_dbfs > 0
              }
            }
          }
        }
      }
    }
  }

  component PluginExpander: ExpanderBox {
    id: pluginExpander

    required property string iconName
    required property PluginGroup model

    icon.source: ResourceManager.getIconUrl("zrythm-dark", iconName)

    frameContentItem: PluginSlotList {
      implicitHeight: contentHeight
      pluginGroup: pluginExpander.model
      pluginImporter: root.pluginImporter
      pluginOperator: root.pluginOperator
      track: root.track
      trackSelectionModel: root.trackSelectionModel

      onPluginClicked: function (plugin: Plugin) {
        root.pluginClicked(plugin);
      }
    }
  }
}
