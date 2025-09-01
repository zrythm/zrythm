// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  required property TrackCollection trackCollection
  property int levelKnobSize: 48
  property int monitorKnobSize: 78

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

        ToolTip.text: qsTr("Currently soloed tracks")
        horizontalAlignment: Text.AlignHCenter
        text: "<small>" + qsTr("%1 soloed").arg(root.trackCollection.numSoloedTracks) + "</small>"
        textFormat: Text.RichText
      }

      Button {
        id: soloingBtn

        ToolTip.text: qsTr("Unsolo all tracks")
        ToolTip.visible: hovered
        enabled: false
        icon.name: "unsolo"
      }
    }

    // Muted tracks
    ColumnLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: 4

      Label {
        id: mutedTracksLbl

        ToolTip.text: qsTr("Currently muted tracks")
        horizontalAlignment: Text.AlignHCenter
        text: "<small>" + qsTr("%1 muted").arg(root.trackCollection.numMutedTracks) + "</small>"
        textFormat: Text.RichText
      }

      Button {
        id: mutingBtn

        ToolTip.text: qsTr("Unmute all tracks")
        enabled: false
        icon.name: "unmute"
      }
    }

    // Listened tracks
    ColumnLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: 4

      Label {
        id: listenedTracksLbl

        ToolTip.text: qsTr("Currently listened tracks")
        horizontalAlignment: Text.AlignHCenter
        text: "<small>" + qsTr("%1 listened").arg(root.trackCollection.numListenedTracks) + "</small>"
        textFormat: Text.RichText
      }

      Button {
        id: listeningBtn

        ToolTip.text: qsTr("Unlisten all tracks")
        enabled: false
        icon.name: "unlisten"
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

      ToolTip.text: qsTr("Sum to mono")
      font.pointSize: 9
      text: qsTr("Mono")
    }

    CheckBox {
      id: dimToggle

      ToolTip.text: qsTr("Dim output")
      font.pointSize: 9
      text: qsTr("Dim")
    }

    CheckBox {
      id: muteToggle

      ToolTip.text: qsTr("Mute output")
      font.pointSize: 9
      text: qsTr("Mute")
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
