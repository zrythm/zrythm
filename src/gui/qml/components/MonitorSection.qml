// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  property int levelKnobSize: 48
  property int monitorKnobSize: 78
  required property TrackCollection trackCollection

  Layout.fillWidth: true
  spacing: 8

  // Track status row (soloed, muted, listened)
  RowLayout {
    id: trackStatusRow

    Layout.alignment: Qt.AlignHCenter
    spacing: 8

    // Soloed tracks
    ColumnLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: 4

      Label {
        id: soloedTracksLbl

        horizontalAlignment: Text.AlignHCenter
        text: "<small>" + qsTr("%1 soloed").arg(root.trackCollection.numSoloedTracks) + "</small>"
        textFormat: Text.RichText

        ToolTip {
          text: qsTr("Currently soloed tracks")
        }
      }

      Button {
        id: soloingBtn

        enabled: false
        icon.name: "unsolo"

        ToolTip {
          text: qsTr("Unsolo all tracks")
        }
      }
    }

    // Muted tracks
    ColumnLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: 4

      Label {
        id: mutedTracksLbl

        horizontalAlignment: Text.AlignHCenter
        text: "<small>" + qsTr("%1 muted").arg(root.trackCollection.numMutedTracks) + "</small>"
        textFormat: Text.RichText

        ToolTip {
          text: qsTr("Currently muted tracks")
        }
      }

      Button {
        id: mutingBtn

        enabled: false
        icon.name: "unmute"

        ToolTip {
          text: qsTr("Unmute all tracks")
        }
      }
    }

    // Listened tracks
    ColumnLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: 4

      Label {
        id: listenedTracksLbl

        horizontalAlignment: Text.AlignHCenter
        text: "<small>" + qsTr("%1 listened").arg(root.trackCollection.numListenedTracks) + "</small>"
        textFormat: Text.RichText

        ToolTip {
          text: qsTr("Currently listened tracks")
        }
      }

      Button {
        id: listeningBtn

        enabled: false
        icon.name: "unlisten"

        ToolTip {
          text: qsTr("Unlisten all tracks")
        }
      }
    }
  }

  // Level controls row (mute, listen, dim)
  RowLayout {
    id: levelControlsRow

    Layout.fillWidth: true
    spacing: 8

    // Mute level
    ColumnLayout {
      Layout.alignment: Qt.AlignHCenter
      Layout.fillWidth: true
      spacing: 4

      Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Mute")
      }

      Knob {
        Layout.alignment: Qt.AlignHCenter
        size: root.levelKnobSize
        zero: 0.5
      }
    }

    // Listen level
    ColumnLayout {
      Layout.alignment: Qt.AlignHCenter
      Layout.fillWidth: true
      spacing: 4

      Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Listen")
      }

      Knob {
        Layout.alignment: Qt.AlignHCenter
        size: root.levelKnobSize
      }
    }

    // Dim level
    ColumnLayout {
      Layout.alignment: Qt.AlignHCenter
      Layout.fillWidth: true
      spacing: 4

      Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Dim")
      }

      Knob {
        Layout.alignment: Qt.AlignHCenter
        size: root.levelKnobSize
      }
    }
  }

  // Monitor buttons row (mono, dim, mute)
  RowLayout {
    id: monitorButtonsRow

    Layout.alignment: Qt.AlignHCenter
    Layout.leftMargin: 2
    Layout.rightMargin: 2
    spacing: 2

    CheckBox {
      id: monoToggle

      font.pointSize: 9
      text: qsTr("Mono")

      ToolTip {
        text: qsTr("Sum to mono")
      }
    }

    CheckBox {
      id: dimToggle

      font.pointSize: 9
      text: qsTr("Dim")

      ToolTip {
        text: qsTr("Dim output")
      }
    }

    CheckBox {
      id: muteToggle

      font.pointSize: 9
      text: qsTr("Mute")

      ToolTip {
        text: qsTr("Mute output")
      }
    }
  }

  // Monitor level
  ColumnLayout {
    id: monitorLevelColumn

    Layout.alignment: Qt.AlignHCenter
    spacing: 4

    Label {
      Layout.fillWidth: true
      horizontalAlignment: Text.AlignHCenter
      text: qsTr("Monitor")
    }

    Knob {
      Layout.alignment: Qt.AlignHCenter
      size: root.monitorKnobSize
    }
  }

  // Output section (replaced with device settings button)
  ColumnLayout {
    Layout.alignment: Qt.AlignHCenter

    Button {
      // onClicked handler will be wired up externally
      font.pointSize: 9
      text: qsTr("Device Settings")
    }
  }
}
