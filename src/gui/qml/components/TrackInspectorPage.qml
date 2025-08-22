// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  required property Track track

  ExpanderBox {
    Layout.fillWidth: true
    title: "Track Properties"

    contentItem: ColumnLayout {
      Label {
        text: root.track.name
      }

      Label {
        text: root.track.comment
      }

      Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 50
        color: root.track.color
      }
    }
  }

  Loader {
    Layout.fillWidth: true
    active: root.track.channel !== null && root.track.channel.inserts !== null

    sourceComponent: ExpanderBox {
      title: "Inserts"

      contentItem: ListView {
        model: root.track.channel.inserts

        delegate: PluginSlotView {
          track: root.track
        }
      }
    }
  }

  Loader {
    Layout.fillWidth: true
    active: root.track.channel !== null

    sourceComponent: ExpanderBox {
      title: "Fader"

      contentItem: ColumnLayout {
        BalanceControl {
          Layout.fillWidth: true
          balanceParameter: root.track.channel.fader.balance
        }

        RowLayout {
          Layout.fillWidth: true

          FaderButtons {
            fader: root.track.channel.fader
            track: root.track
          }

          FaderControl {
            faderGain: root.track.channel.fader.gain
          }

          TrackMeters {
            id: meters

            Layout.fillHeight: true
            Layout.fillWidth: false
            channel: root.track.channel
          }

          Label {
            id: peakLabel

            readonly property real peak_in_dbfs: QmlUtils.amplitudeToDbfs(meters.currentPeak)

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
              value: Style.dangerColor
              when: peakLabel.peak_in_dbfs > 0
            }
          }
        }
      }
    }
  }
}
