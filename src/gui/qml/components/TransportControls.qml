// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

RowLayout {
  id: root

  required property var tempoMap
  required property var transport

  LinkedButtons {
    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-backward-large-symbolic.svg")

      onClicked: transport.moveBackward()
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-forward-large-symbolic.svg")

      onClicked: transport.moveForward()
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "skip-backward-large-symbolic.svg")
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", (transport.playState == 1 ? "pause" : "play") + "-large-symbolic.svg")

      onClicked: {
        transport.isRolling() ? transport.requestPause(true) : transport.requestRoll(true);
      }
    }

    RecordButton {
      id: recordButton

      checked: transport.recordEnabled

      onCheckedChanged: {
        transport.recordEnabled = checked;
      }
    }

    Button {
      checkable: true
      checked: transport.loopEnabled
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "loop-arrow-symbolic.svg")

      onCheckedChanged: {
        transport.loopEnabled = checked;
      }
    }
  }

  RowLayout {
    spacing: 2

    EditableValueDisplay {
      label: "bpm"
      value: root.tempoMap.tempoAtTick(transport.playhead.ticks).toFixed(2)
    }

    EditableValueDisplay {
      id: timeDisplay

      label: "time"
      minValueHeight: timeTextMetrics.height
      minValueWidth: timeTextMetrics.width
      value: root.tempoMap.getMusicalPositionString(transport.playhead.ticks)

      TextMetrics {
        id: timeTextMetrics

        font: Style.semiBoldTextFont
        text: "99.9.9.999"
      }
    }

    EditableValueDisplay {
      label: "sig"
      value: {
        let timeSig = root.tempoMap.timeSignatureAtTick(transport.playhead.ticks);
        return `${timeSig.numerator}/${timeSig.denominator}`;
      }
    }
  }
}
