// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmActions
import ZrythmStyle

/**
 * Compact editor for the native Envelope Follower modulator node.
 *
 * The host node supplies the parameter objects, resolved through the
 * engine's stable parameter IDs, so edits go through the normal user-edit
 * APIs (undo, automation and MIDI mapping included). Knob values are the
 * parameters' normalized 0-1 representations; the readouts show the
 * converted times.
 */
Item {
  id: root

  required property var attackParameter
  required property var releaseParameter
  required property ProcessorParameterOperator parameterOperator

  signal editActiveChanged(bool active)

  component TimeKnob: ColumnLayout {
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
      size: 28
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
      elide: Text.ElideRight
      font: ZrythmTheme.xSmallTextFont
      text: knobColumn.hasParameter
        ? knobColumn.title + " "
            + knobColumn.formatTime(knobColumn.parameter) : ""
    }

    function formatTime(parameter: var): string {
      const seconds =
        parameter.range.convertFrom0To1(parameter.baseValue);
      return seconds >= 1 ? seconds.toFixed(2) + " s"
        : Math.round(seconds * 1000) + " ms";
    }
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 12
    anchors.rightMargin: 12
    spacing: 10

    TimeKnob {
      Layout.fillWidth: true
      parameter: root.attackParameter
      title: qsTr("Attack")
    }

    TimeKnob {
      Layout.fillWidth: true
      parameter: root.releaseParameter
      title: qsTr("Release")
    }
  }
}
