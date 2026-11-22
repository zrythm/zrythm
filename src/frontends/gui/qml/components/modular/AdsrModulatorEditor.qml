// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmActions
import ZrythmStyle

/**
 * Compact editor for the native ADSR modulator node.
 *
 * The host node supplies the parameter objects, resolved through the
 * engine's stable parameter IDs, so edits go through the normal user-edit
 * APIs (undo, automation and MIDI mapping included). Knob values are the
 * parameters' normalized 0-1 representations.
 */
Item {
  id: root

  required property var attackParameter
  required property var decayParameter
  required property var sustainParameter
  required property var releaseParameter
  required property ProcessorParameterOperator parameterOperator

  signal editActiveChanged(bool active)

  component AdsrKnob: ColumnLayout {
    id: knobColumn

    property var parameter: null
    property string title: ""

    readonly property bool hasParameter: parameter !== null
      && parameter !== undefined

    spacing: 0

    Knob {
      Layout.alignment: Qt.AlignHCenter
      defaultValue: knobColumn.hasParameter ? knobColumn.parameter.range.defaultNormalizedValue : 0
      enabled: knobColumn.hasParameter
      flat: true
      maxValue: 1
      minValue: 0
      size: 26
      value: knobColumn.hasParameter ? knobColumn.parameter.baseValue : 0
      zero: 0

      onDraggingChanged: {
        if (knobColumn.hasParameter) {
          if (dragging)
            knobColumn.parameter.beginUserGesture();
          else
            knobColumn.parameter.endUserGesture();
        }
        root.editActiveChanged(dragging);
      }
      onValueModified: newValue => {
        if (knobColumn.hasParameter)
          root.parameterOperator.setValue(knobColumn.parameter, newValue);
      }
    }

    Label {
      Layout.alignment: Qt.AlignHCenter
      color: ZrythmTheme.placeholderTextColor
      font: ZrythmTheme.xSmallTextFont
      text: knobColumn.title
    }
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 12
    anchors.rightMargin: 12
    spacing: 4

    AdsrKnob {
      Layout.fillWidth: true
      parameter: root.attackParameter
      title: qsTr("A")
    }
    AdsrKnob {
      Layout.fillWidth: true
      parameter: root.decayParameter
      title: qsTr("D")
    }
    AdsrKnob {
      Layout.fillWidth: true
      parameter: root.sustainParameter
      title: qsTr("S")
    }
    AdsrKnob {
      Layout.fillWidth: true
      parameter: root.releaseParameter
      title: qsTr("R")
    }
  }
}
