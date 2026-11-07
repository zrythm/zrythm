// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ScrollView {
  id: root

  required property AudioEngine audioEngine
  required property ControlRoom controlRoom
  required property DeviceManager deviceManager
  required property PortObservationManager portObservationManager
  readonly property color quietInkColor: Qt.alpha(ZrythmTheme.textColor, 0.62)
  required property TrackCollection trackCollection
  required property TrackCollectionOperator trackCollectionOperator

  // Formats a linear amplitude as dB text, bottoming out at -infinity.
  function dbText(amplitude: real): string {
    const db = 20 * Math.log10(amplitude);
    if (db < -100)
      return qsTr("-∞ dB");

    return db.toFixed(1) + qsTr(" dB");
  }

  Layout.fillWidth: true
  clip: true
  contentWidth: availableWidth

  ColumnLayout {
    spacing: 0
    width: root.width

    ColumnLayout {
      id: contentLayout

      Layout.fillWidth: true
      Layout.margins: 12
      spacing: 8

      ActionRowGroup {
        Layout.fillWidth: true
        subtitle: qsTr("Main monitor volume")
        title: qsTr("Output level")

        ActionRow {
          hoverBaseColor: ZrythmTheme.cardColor
          title: qsTr("Volume")

          RowLayout {
            spacing: 8

            Slider {
              id: outputSlider

              Accessible.name: qsTr("Main monitor volume")
              Layout.maximumWidth: 110
              Layout.minimumWidth: 110
              Layout.preferredWidth: 110
              from: 0
              to: 1
              value: root.controlRoom.monitorFader.gain.baseValue

              onMoved: root.controlRoom.monitorFader.gain.setBaseValueByUser(value)
            }

            Label {
              Layout.maximumWidth: 52
              Layout.minimumWidth: 52
              Layout.preferredWidth: 52
              font.pointSize: 8
              horizontalAlignment: Text.AlignRight
              text: root.dbText(root.controlRoom.monitorFader.gain.range.convertFrom0To1(root.controlRoom.monitorFader.gain.baseValue))
            }

            ToolButton {
              Accessible.name: qsTr("Reset monitor level")
              ToolTip.text: qsTr("Reset to unity")
              ToolTip.visible: hovered
              display: AbstractButton.IconOnly
              icon.height: 14
              icon.source: ResourceManager.getIconUrl("box-icons", "reset.svg")
              icon.width: 14

              onClicked: root.controlRoom.monitorFader.gain.setBaseValueByUser(root.controlRoom.monitorFader.gain.range.defaultNormalizedValue)
            }
          }
        }

        ChannelMeterRow {
          channelIndex: 0
          channelTitle: qsTr("L")
        }

        ChannelMeterRow {
          channelIndex: 1
          channelTitle: qsTr("R")
        }

        // Output processing toggles as compact latching buttons.
        RowLayout {
          Layout.bottomMargin: 8
          Layout.fillWidth: true
          Layout.leftMargin: 6
          Layout.rightMargin: 6
          Layout.topMargin: 4
          spacing: 8

          Button {
            Layout.fillWidth: true
            ToolTip.text: qsTr("Sum the left and right channels")
            ToolTip.visible: hovered
            checkable: true
            checked: root.controlRoom.monitorFader.monoToggle.range.isToggled(root.controlRoom.monitorFader.monoToggle.baseValue)
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "monitor-mono.svg")
            text: qsTr("Mono")

            onToggled: root.controlRoom.monitorFader.monoToggle.setBaseValueByUser(checked ? 1.0 : 0.0)
          }

          Button {
            Layout.fillWidth: true
            ToolTip.text: qsTr("Attenuate the output by the dim level")
            ToolTip.visible: hovered
            checkable: true
            checked: root.controlRoom.dimOutput
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "monitor-dim.svg")
            text: qsTr("Dim")

            onToggled: root.controlRoom.dimOutput = checked
          }

          Button {
            Layout.fillWidth: true
            ToolTip.text: qsTr("Mute the output at the muted-track level")
            ToolTip.visible: hovered
            checkable: true
            checked: root.controlRoom.monitorFader.mute.range.isToggled(root.controlRoom.monitorFader.mute.baseValue)
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "monitor-mute.svg")
            text: qsTr("Mute")

            onToggled: root.controlRoom.monitorFader.mute.setBaseValueByUser(checked ? 1.0 : 0.0)
          }
        }
      }

      ActionRowGroup {
        Layout.fillWidth: true
        title: qsTr("Track states")

        ActionRow {
          hoverBaseColor: ZrythmTheme.cardColor
          subtitle: root.trackCollection.numSoloedTracks === 1 ? qsTr("1 track soloed") : qsTr("%1 tracks soloed").arg(root.trackCollection.numSoloedTracks)
          title: qsTr("Solo")

          Button {
            enabled: root.trackCollection.numSoloedTracks > 0
            text: qsTr("Reset")

            onClicked: root.trackCollectionOperator.unsoloAllTracks()
          }
        }

        ActionRow {
          hoverBaseColor: ZrythmTheme.cardColor
          subtitle: root.trackCollection.numMutedTracks === 1 ? qsTr("1 track muted") : qsTr("%1 tracks muted").arg(root.trackCollection.numMutedTracks)
          title: qsTr("Mute")

          Button {
            enabled: root.trackCollection.numMutedTracks > 0
            text: qsTr("Reset")

            onClicked: root.trackCollectionOperator.unmuteAllTracks()
          }
        }

        ActionRow {
          hoverBaseColor: ZrythmTheme.cardColor
          subtitle: root.trackCollection.numListenedTracks === 1 ? qsTr("1 track listened") : qsTr("%1 tracks listened").arg(root.trackCollection.numListenedTracks)
          title: qsTr("Listen")

          Button {
            enabled: root.trackCollection.numListenedTracks > 0
            text: qsTr("Reset")

            onClicked: root.trackCollectionOperator.unlistenAllTracks()
          }
        }
      }

      ExpanderBox {
        Layout.fillWidth: true
        expanded: false
        title: qsTr("Advanced levels")

        frameContentItem: ColumnLayout {
          spacing: 4

          ParamLevelRow {
            Layout.fillWidth: true
            param: root.controlRoom.muteVolume
            valueSubtitle: qsTr("Applied while mute is active")
            valueTitle: qsTr("Muted track")
          }

          ParamLevelRow {
            Layout.fillWidth: true
            param: root.controlRoom.listenVolume
            valueSubtitle: qsTr("Applied to listened tracks")
            valueTitle: qsTr("Listen")
          }

          ParamLevelRow {
            Layout.fillWidth: true
            param: root.controlRoom.dimVolume
            valueSubtitle: qsTr("Applied while dim is active")
            valueTitle: qsTr("Dim")
          }
        }
      }

      Button {
        Accessible.name: qsTr("Open audio device settings")
        Layout.fillWidth: true
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "settings-symbolic.svg")
        text: qsTr("Device Settings")

        onClicked: root.deviceManager.showDeviceSelector()
      }
    }
  }

  component ChannelMeterRow: ActionRow {
    id: channelMeterRow

    property int channelIndex: 0
    property string channelTitle: ""

    hoverBaseColor: ZrythmTheme.cardColor
    title: channelMeterRow.channelTitle

    RowLayout {
      spacing: 8

      Meter {
        id: channelMeter

        Layout.maximumWidth: 150
        Layout.minimumWidth: 150
        Layout.preferredWidth: 150
        algorithm: MeterProcessor.DigitalPeak
        audioEngine: root.audioEngine
        channel: channelMeterRow.channelIndex
        horizontal: true
        implicitHeight: 10
        k20Colors: true
        port: root.controlRoom.monitorFader.stereoOutPort
        portObservationManager: root.portObservationManager
        segmentCount: 22
      }

      Label {
        Layout.maximumWidth: 52
        Layout.minimumWidth: 52
        Layout.preferredWidth: 52
        color: root.quietInkColor
        font.pointSize: 8
        horizontalAlignment: Text.AlignRight
        text: root.dbText(channelMeter.meterProcessor.currentAmplitude)
      }
    }
  }
  component ParamLevelRow: ActionRow {
    id: paramLevelRow

    required property ProcessorParameter param
    property string valueSubtitle: ""
    property string valueTitle: ""

    hoverBaseColor: ZrythmTheme.cardColor
    subtitle: paramLevelRow.valueSubtitle
    title: paramLevelRow.valueTitle

    RowLayout {
      spacing: 8

      Slider {
        Accessible.name: paramLevelRow.valueTitle
        Layout.maximumWidth: 110
        Layout.minimumWidth: 110
        Layout.preferredWidth: 110
        from: 0
        to: 1
        value: paramLevelRow.param.baseValue

        onMoved: paramLevelRow.param.setBaseValueByUser(value)
      }

      Label {
        Layout.maximumWidth: 52
        Layout.minimumWidth: 52
        Layout.preferredWidth: 52
        color: root.quietInkColor
        font.pointSize: 8
        horizontalAlignment: Text.AlignRight
        text: root.dbText(paramLevelRow.param.range.convertFrom0To1(paramLevelRow.param.baseValue))
      }
    }
  }
}
