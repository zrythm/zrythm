// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle
import Zrythm

// Channel state controls for a mixer strip, stacked as pills next to
// the fader: the primary pill carries the channel states (mute, solo,
// record, listen) and the secondary pill the signal toggles (mono,
// phase, monitor). The secondary pill is ghosted at rest and
// materializes when any of its toggles is active. The channel settings
// button stays standalone below the pills.
ColumnLayout {
  id: root

  required property Fader fader
  required property Track track

  readonly property bool monoOn: root.fader.monoToggle ? root.fader.monoToggle.baseValue > 0.5 : false
  // Monitor cycles Off -> On -> Auto; Auto additionally shows the
  // corner dot on the chip
  readonly property int monitorMode: root.track.monitorParam ? root.track.monitorParam.range.enumIndex(root.track.monitorParam.baseValue) : -1
  readonly property bool phaseOn: root.fader.swapPhaseToggle ? root.fader.swapPhaseToggle.baseValue > 0.5 : false
  // The secondary pill materializes when any of its toggles is active
  readonly property bool secondaryActive: root.monoOn || root.phaseOn || root.monitorMode > 0

  spacing: 8

  LinkedButtons {
    id: primaryPill

    vertical: true

    ChannelStateButton {
      Accessible.name: qsTr("Mute")
      checkedFill: palette.dark
      checked: root.fader.mute ? root.fader.mute.baseValue > 0.5 : false
      text: "M"
      visible: root.fader.mute !== null

      // External value syncs that flip checked re-enter these handlers;
      // the write-back is a no-op then (value unchanged), which only
      // holds for binary 0/1 toggles like these
      onCheckedChanged: root.fader.mute?.setBaseValueByUser(checked ? 1.0 : 0.0)

      ToolTip {
        text: qsTr("Mute")
      }
    }

    ChannelStateButton {
      Accessible.name: qsTr("Solo")
      checkedFill: ZrythmTheme.soloGreenColor
      checked: root.fader.solo ? root.fader.solo.baseValue > 0.5 : false
      text: "S"

      onCheckedChanged: root.fader.solo?.setBaseValueByUser(checked ? 1.0 : 0.0)

      ToolTip {
        text: qsTr("Solo")
      }
    }

    ChannelStateButton {
      Accessible.name: qsTr("Record")
      checkedFill: ZrythmTheme.recordColor
      checked: root.track.recordingParam?.range.isToggled(root.track.recordingParam.baseValue) ?? false
      glyphPixelSize: 10
      restInk: ZrythmTheme.recordTextColor
      text: "●"
      visible: root.track.recordingParam !== null

      onClicked: {
        const param = root.track.recordingParam;
        param.setBaseValueByUser(param.range.isToggled(param.baseValue) ? 0.0 : 1.0);
      }

      ToolTip {
        text: qsTr("Record")
      }
    }

    ChannelStateButton {
      Accessible.name: qsTr("Listen")
      checkedFill: palette.dark
      checked: root.fader.listen ? root.fader.listen.baseValue > 0.5 : false
      text: "L"
      visible: root.fader.listen !== null

      onCheckedChanged: root.fader.listen?.setBaseValueByUser(checked ? 1.0 : 0.0)

      ToolTip {
        text: qsTr("Listen")
      }
    }
  }

  LinkedButtons {
    id: secondaryPill

    vertical: true

    ChannelStateButton {
      Accessible.name: qsTr("Mono compatibility")
      checkable: false
      checked: root.monoOn
      checkedFill: palette.dark
      ghost: !root.secondaryActive
      icon.source: ResourceManager.getIconUrl("codicons", "merge.svg")
      visible: root.fader.monoToggle !== null

      onClicked: {
        root.fader.monoToggle?.setBaseValueByUser(root.monoOn ? 0.0 : 1.0);
      }

      ToolTip {
        text: qsTr("Mono compatibility")
      }
    }

    ChannelStateButton {
      Accessible.name: qsTr("Swap phase")
      checkable: false
      checked: root.phaseOn
      checkedFill: palette.dark
      ghost: !root.secondaryActive
      text: "Ø"

      onClicked: {
        root.fader.swapPhaseToggle?.setBaseValueByUser(root.phaseOn ? 0.0 : 1.0);
      }

      ToolTip {
        text: qsTr("Swap phase")
      }
    }

    ChannelStateButton {
      Accessible.name: qsTr("Monitor")
      checkable: false
      checked: root.monitorMode > 0
      checkedFill: palette.dark
      ghost: !root.secondaryActive
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "audition.svg")
      showActiveDot: root.monitorMode === 2
      visible: root.track.monitorParam !== null

      onClicked: {
        if (!root.track.monitorParam)
          return;
        const next = (root.monitorMode + 1) % root.track.monitorParam.range.enumCount();
        root.track.monitorParam.setBaseValueByUser(root.track.monitorParam.range.normalizedEnumValue(next));
      }

      ToolTip {
        text: {
          if (!root.track.monitorParam)
            return "";
          return qsTr("Monitor: %1").arg(root.track.monitorParam.range.enumLabel(root.monitorMode));
        }
      }
    }
  }

  ChannelStateButton {
    Accessible.name: qsTr("Channel settings")
    checkable: false
    ghost: true
    icon.source: ResourceManager.getIconUrl("gnome-icon-library", "settings-symbolic.svg")

    onClicked:
    // TODO: Open channel settings dialog
    {}

    ToolTip {
      text: qsTr("Channel settings")
    }
  }
}
