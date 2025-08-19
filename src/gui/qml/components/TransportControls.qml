// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

RowLayout {
  id: root

  required property TempoMap tempoMap
  required property Transport transport

  LinkedButtons {
    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-backward-large-symbolic.svg")

      onClicked: root.transport.moveBackward()
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-forward-large-symbolic.svg")

      onClicked: root.transport.moveForward()
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "skip-backward-large-symbolic.svg")
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", (root.transport.playState == 1 ? "pause" : "play") + "-large-symbolic.svg")

      onClicked: {
        root.transport.isRolling() ? root.transport.requestPause(true) : root.transport.requestRoll(true);
      }
    }

    RecordButton {
      id: recordButton

      checked: root.transport.recordEnabled

      onCheckedChanged: {
        root.transport.recordEnabled = checked;
      }
    }

    Button {
      checkable: true
      checked: root.transport.loopEnabled
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "loop-arrow-symbolic.svg")

      onCheckedChanged: {
        root.transport.loopEnabled = checked;
      }
    }
  }

  RowLayout {
    spacing: 2

    EditableValueDisplay {
      label: "bpm"
      value: root.tempoMap.tempoAtTick(root.transport.playhead.ticks).toFixed(2)
    }

    EditableValueDisplay {
      id: timeDisplay

      label: "time"
      minValueHeight: timeTextMetrics.height
      minValueWidth: timeTextMetrics.width
      value: root.tempoMap.getMusicalPositionString(root.transport.playhead.ticks)

      TextMetrics {
        id: timeTextMetrics

        font: Style.semiBoldTextFont
        text: "99.9.9.999"
      }
    }

    EditableValueDisplay {
      label: "sig"
      value: {
        const timeSigNumerator = root.tempoMap.timeSignatureNumeratorAtTick(root.transport.playhead.ticks);
        const timeSigDenominator = root.tempoMap.timeSignatureDenominatorAtTick(root.transport.playhead.ticks);
        return `${timeSigNumerator}/${timeSigDenominator}`;
      }
    }
  }
}
