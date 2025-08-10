// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick.Controls
import Zrythm 1.0
import ZrythmStyle 1.0

ToolBar {
  id: root

  required property TrackFactory trackFactory
  required property Tracklist tracklist

  ToolButton {
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "add.svg")

    onClicked: {
      addMenu.popup();
    }

    ToolTip {
      text: qsTr("Add track")
    }

    Menu {
      id: addMenu

      MenuItem {
        text: qsTr("Add _MIDI Track")

        onTriggered: root.trackFactory.addEmptyTrackFromType(Track.Midi)
      }

      MenuItem {
        text: qsTr("Add Audio Track")

        onTriggered: root.trackFactory.addEmptyTrackFromType(Track.Audio)
      }

      MenuSeparator {
      }

      MenuItem {
        text: qsTr("Import File...")
      }

      MenuSeparator {
      }

      MenuItem {
        text: qsTr("Add Audio FX Track")

        onTriggered: root.trackFactory.addEmptyTrackFromType(Track.AudioBus)
      }

      MenuItem {
        text: qsTr("Add MIDI FX Track")

        onTriggered: root.trackFactory.addEmptyTrackFromType(Track.MidiBus)
      }

      MenuSeparator {
      }

      MenuItem {
        text: qsTr("Add Audio Group Track")

        onTriggered: root.trackFactory.addEmptyTrackFromType(Track.AudioGroup)
      }

      MenuItem {
        text: qsTr("Add MIDI Group Track")

        onTriggered: root.trackFactory.addEmptyTrackFromType(Track.MidiGroup)
      }

      MenuSeparator {
      }

      MenuItem {
        text: qsTr("Add Folder Track")

        onTriggered: root.trackFactory.addEmptyTrackFromType(Track.Folder)
      }
    }
  }
}
