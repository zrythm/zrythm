// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle
import Zrythm

// Stereo balance as a center-detent knob between `L`/`R` labels with the
// position as text (`C`, `L50`, `R50`); shared by the mixer strips and
// the inspector's fader section.
Item {
  id: root

  required property ProcessorParameter balanceParameter
  required property ProcessorParameterOperator parameterOperator

  readonly property string panText: {
    const value = root.balanceParameter.baseValue;
    const pct = Math.round(Math.abs(value - 0.5) / 0.5 * 100);
    if (pct === 0)
      return qsTr("C");
    return value < 0.5 ? qsTr("L%1").arg(pct) : qsTr("R%1").arg(pct);
  }

  implicitHeight: balanceLayout.implicitHeight
  implicitWidth: balanceLayout.implicitWidth

  RowLayout {
    id: balanceLayout

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    spacing: 4

    Label {
      color: Qt.alpha(palette.windowText, 0.4)
      font: ZrythmTheme.xxSmallTextFont
      text: "L"
    }

    Knob {
      Layout.alignment: Qt.AlignVCenter

      defaultValue: 0.5
      maxValue: 1
      minValue: 0
      size: 26
      value: root.balanceParameter.baseValue
      zero: 0.5

      onValueModified: function (newValue) {
        root.parameterOperator.setValue(root.balanceParameter, newValue);
      }
    }

    Label {
      color: Qt.alpha(palette.windowText, 0.4)
      font: ZrythmTheme.xxSmallTextFont
      text: "R"
    }

    Item {
      Layout.fillWidth: true
    }

    Label {
      font: ZrythmTheme.smallTextFont
      text: root.panText
    }
  }
}
