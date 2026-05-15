// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
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
    checked: root.track.recordingParam?.range.isToggled(root.track.recordingParam.baseValue) ?? false
    icon.color: checked ? "#ff0000" : palette.buttonText
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "media-record.svg")
    visible: root.track.recordingParam !== null

    onClicked: {
      const param = root.track.recordingParam;
      param.baseValue = param.range.isToggled(param.baseValue) ? 0.0 : 1.0;
    }

    ToolTip {
      text: qsTr("Record")
    }
  }

  // Monitor mode button (cycles Off → On → Auto)
  ToolButton {
    id: monitorButton

    property ProcessorParameter monitorParam: root.track.monitorParam

    function updateColor(): void {
      if (!monitorParam)
        return;
      const idx = monitorParam.range.enumIndex(monitorParam.baseValue);
      switch (idx) {
      case 0:
        monitorButton.icon.color = palette.mid;
        break;
      case 1:
        monitorButton.icon.color = palette.highlight;
        break;
      default:
        monitorButton.icon.color = palette.buttonText;
        break;
      }
    }

    Layout.fillHeight: true
    Layout.fillWidth: true
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "audition.svg")
    visible: monitorParam !== null

    Component.onCompleted: updateColor()
    onClicked: {
      if (!monitorParam)
        return;
      const idx = monitorParam.range.enumIndex(monitorParam.baseValue);
      const next = (idx + 1) % monitorParam.range.enumCount();
      monitorParam.baseValue = monitorParam.range.normalizedEnumValue(next);
    }
    onMonitorParamChanged: updateColor()

    Connections {
      function onBaseValueChanged() {
        monitorButton.updateColor();
      }

      enabled: monitorButton.monitorParam !== null
      target: monitorButton.monitorParam
    }

    ToolTip {
      text: {
        if (!root.track.monitorParam)
          return "";
        const idx = root.track.monitorParam.range.enumIndex(root.track.monitorParam.baseValue);
        return qsTr("Monitor: %1").arg(root.track.monitorParam.range.enumLabel(idx));
      }
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
    icon.source: ResourceManager.getIconUrl("gnome-icon-library", "settings-symbolic.svg")

    onClicked:
    // TODO: Open channel settings dialog
    {}

    ToolTip {
      text: qsTr("Channel settings")
    }
  }
}
