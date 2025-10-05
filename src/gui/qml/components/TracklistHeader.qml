// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick.Controls
import ZrythmGui
import Zrythm
import ZrythmStyle

ZrythmToolBar {
  id: root

  property alias clipLauncherVisible: clipLauncherVisibleButton.checked
  property alias tempoMapVisible: tempoMeterButton.checked
  property alias timelineVisible: timelineVisibleButton.checked
  required property TrackCreator trackCreator
  required property Tracklist tracklist

  leftItems: [
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

          onTriggered: root.trackCreator.addEmptyTrackFromType(Track.Midi)
        }

        MenuItem {
          text: qsTr("Add Audio Track")

          onTriggered: root.trackCreator.addEmptyTrackFromType(Track.Audio)
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

          onTriggered: root.trackCreator.addEmptyTrackFromType(Track.AudioBus)
        }

        MenuItem {
          text: qsTr("Add MIDI FX Track")

          onTriggered: root.trackCreator.addEmptyTrackFromType(Track.MidiBus)
        }

        MenuSeparator {
        }

        MenuItem {
          text: qsTr("Add Audio Group Track")

          onTriggered: root.trackCreator.addEmptyTrackFromType(Track.AudioGroup)
        }

        MenuItem {
          text: qsTr("Add MIDI Group Track")

          onTriggered: root.trackCreator.addEmptyTrackFromType(Track.MidiGroup)
        }

        MenuSeparator {
        }

        MenuItem {
          text: qsTr("Add Folder Track")

          onTriggered: root.trackCreator.addEmptyTrackFromType(Track.Folder)
        }
      }
    },
    ToolButton {
      id: clipLauncherVisibleButton

      checkable: true
      checked: false
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "mouse-keys-symbolic.svg")

      ToolTip {
        text: qsTr("Clip Launcher")
      }
    },
    ToolButton {
      id: timelineVisibleButton

      checkable: true
      checked: true
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "ruler-angled-symbolic.svg")

      ToolTip {
        text: qsTr("Timeline")
      }
    }
  ]
  rightItems: [
    ToolButton {
      id: tempoMeterButton

      checkable: true
      checked: false
      ToolTip {
      text: qsTr("BPM & Time Signature")
      }

      icon.source: ResourceManager.getIconUrl("zrythm-dark", "bpm-time-signature.svg")
    }
  ]
}
