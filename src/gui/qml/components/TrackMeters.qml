// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

RowLayout {
  id: root

  required property Channel channel

  Loader {
    id: audioMetersLoader

    Layout.fillHeight: true
    Layout.fillWidth: false
    active: root.channel && root.channel.leftAudioOut
    visible: active

    sourceComponent: RowLayout {
      id: meters

      anchors.fill: parent
      spacing: 0

      Meter {
        Layout.fillHeight: true
        Layout.preferredWidth: width
        port: root.channel.leftAudioOut
      }

      Meter {
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
      anchors.fill: parent
      port: root.channel.midiOut
    }
  }
}
