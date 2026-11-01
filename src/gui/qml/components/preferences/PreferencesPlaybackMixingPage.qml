// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import Qt.labs.synchronizer
import QtQuick.Controls
import Zrythm
import ZrythmStyle

ZrythmPreferencesPage {
  id: root

  property AppSettings appSettings: null
  title: qsTr("Playback & Mixing")

  ActionRowGroup {
    title: qsTr("Transport")
    subtitle: qsTr("How playback starts and stops")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Return the playhead to its cue point when playback stops")
      title: qsTr("Return To Cue")

      Switch {
        Synchronizer on checked {
          sourceObject: root.appSettings
          sourceProperty: "transportReturnToCue"
        }
      }
    }

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Start playback when a MIDI note is received")
      title: qsTr("Start Playback On MIDI Input")

      Switch {
        Synchronizer on checked {
          sourceObject: root.appSettings
          sourceProperty: "startPlaybackOnMidiInput"
        }
      }
    }
  }
}
