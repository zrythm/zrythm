// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle
import Zrythm

ColumnLayout {
  id: root

  required property Fader fader
  required property Track track

  spacing: 1

  // Mono compatibility button
  ToolButton {
    id: monoCompatButton

    Layout.fillHeight: true
    Layout.fillWidth: true
    checkable: true
    checked: root.fader.monoToggle ? root.fader.monoToggle.baseValue > 0.5 : false
    icon.color: checked ? palette.highlight : palette.buttonText
    icon.source: ResourceManager.getIconUrl("codicons", "merge.svg")
    visible: root.fader.monoToggle !== null

    Binding {
      property: "baseValue"
      target: root.fader.monoToggle
      value: monoCompatButton.checked ? 1.0 : 0.0
      when: root.fader.monoToggle !== null
    }

    ToolTip {
      text: qsTr("Mono compatibility")
    }
  }

  // Record button
  ToolButton {
    id: recordButton

    Layout.fillHeight: true
    Layout.fillWidth: true
    checkable: true
    checked: root.track.recordableTrackMixin ? root.track.recordableTrackMixin.recording : false
    icon.color: checked ? "#ff0000" : palette.buttonText
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "media-record.svg")
    visible: root.track.recordableTrackMixin !== null

    ToolTip {
      text: qsTr("Record")
    }
  }

  // Solo button
  ToolButton {
    id: soloButton

    Layout.fillHeight: true
    Layout.fillWidth: true
    checkable: true
    checked: root.fader.solo ? root.fader.solo.baseValue > 0.5 : false
    icon.color: checked ? palette.highlight : palette.buttonText
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "solo.svg")

    ToolTip {
      text: qsTr("Solo")
    }

    Binding {
      property: "baseValue"
      target: root.fader.solo
      value: soloButton.checked ? 1.0 : 0.0
      when: root.fader.solo !== null
    }
  }

  // Mute button
  ToolButton {
    id: muteButton

    Layout.fillHeight: true
    Layout.fillWidth: true
    checkable: true
    checked: root.fader.mute ? root.fader.mute.baseValue > 0.5 : false
    icon.color: checked ? "#ff0000" : palette.buttonText
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "mute.svg")

    ToolTip {
      text: qsTr("Mute")
    }

    Binding {
      property: "baseValue"
      target: root.fader.mute
      value: muteButton.checked ? 1.0 : 0.0
      when: root.fader.mute !== null
    }
  }

  // Listen button
  ToolButton {
    id: listenButton

    Layout.fillHeight: true
    Layout.fillWidth: true
    checkable: true
    checked: root.fader.listen ? root.fader.listen.baseValue > 0.5 : false
    icon.color: checked ? palette.highlight : palette.buttonText
    icon.source: ResourceManager.getIconUrl("gnome-icon-library", "headphones-symbolic.svg")

    Binding {
      property: "baseValue"
      target: root.fader.listen
      value: listenButton.checked ? 1.0 : 0.0
      when: root.fader.listen !== null
    }

    ToolTip {
      text: qsTr("Listen")
    }
  }

  // Swap phase button
  ToolButton {
    id: swapPhaseButton

    Layout.fillHeight: true
    Layout.fillWidth: true
    checkable: true
    checked: root.fader.swapPhaseToggle ? root.fader.swapPhaseToggle.baseValue > 0.5 : false
    icon.color: checked ? palette.highlight : palette.buttonText
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "phase.svg")

    Binding {
      property: "baseValue"
      target: root.fader.swapPhaseToggle
      value: swapPhaseButton.checked ? 1.0 : 0.0
      when: root.fader.swapPhaseToggle !== null
    }

    ToolTip {
      text: qsTr("Swap phase")
    }
  }

  // Channel settings button
  ToolButton {
    id: channelSettingsButton

    Layout.fillHeight: true
    Layout.fillWidth: true
    icon.color: palette.buttonText
    icon.source: ResourceManager.getIconUrl("gnome-icon-library", "settings-symbolic.svg")

    onClicked:
    // TODO: Open channel settings dialog
    {}

    ToolTip {
      text: qsTr("Channel settings")
    }
  }
}
