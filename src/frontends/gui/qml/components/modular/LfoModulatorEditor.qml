// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmActions
import ZrythmStyle

/**
 * Compact editor for the native LFO modulator node.
 *
 * The host node supplies the parameter objects, resolved through the
 * engine's stable parameter IDs, so edits go through the normal user-edit
 * APIs (undo, automation and MIDI mapping included). Knob values are the
 * parameters' normalized 0-1 representations.
 */
Item {
  id: root

  required property var rateParameter
  required property var syncParameter
  required property var divisionParameter
  required property var waveformParameter
  required property ProcessorParameterOperator parameterOperator

  signal editActiveChanged(bool active)

  readonly property bool hasParameters: rateParameter !== null
    && rateParameter !== undefined && syncParameter !== null
    && syncParameter !== undefined && divisionParameter !== null
    && divisionParameter !== undefined && waveformParameter !== null
    && waveformParameter !== undefined
  readonly property bool tempoSynced: root.hasParameters
    && root.syncParameter.range.isToggled(root.syncParameter.baseValue)

  component EnumComboBox: ComboBox {
    id: combo

    property var parameter: null

    function syncCurrentIndex(): void {
      currentIndex = parameter !== null
        ? parameter.range.enumIndex(parameter.baseValue) : -1;
    }

    font: ZrythmTheme.smallTextFont
    model: {
      if (parameter === null)
        return [];
      const labels = [];
      for (let i = 0; i < parameter.range.enumCount(); ++i) {
        labels.push(parameter.range.enumLabel(i));
      }
      return labels;
    }

    Component.onCompleted: syncCurrentIndex()
    onParameterChanged: syncCurrentIndex()

    onActivated: function(index: int): void {
      if (parameter !== null) {
        root.parameterOperator.setValue(
          parameter, parameter.range.normalizedEnumValue(index));
      }
    }

    Connections {
      function onBaseValueChanged(): void {
        combo.syncCurrentIndex();
      }

      target: combo.parameter
    }
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 12
    anchors.rightMargin: 12
    spacing: 6

    CheckBox {
      text: qsTr("Sync")
      font: ZrythmTheme.smallTextFont
      checked: root.tempoSynced
      enabled: root.hasParameters

      onToggled: {
        if (root.hasParameters)
          root.parameterOperator.setValue(
            root.syncParameter, checked ? 1 : 0);
      }
    }

    EnumComboBox {
      Layout.preferredWidth: 76
      parameter: root.waveformParameter
    }

    // Only one of the rate controls applies at a time; showing just it
    // keeps the row inside the node width.
    EnumComboBox {
      Layout.preferredWidth: 52
      visible: root.tempoSynced
      parameter: root.divisionParameter
    }

    Knob {
      id: rateKnob

      defaultValue: root.hasParameters ? root.rateParameter.range.defaultNormalizedValue : 0
      enabled: root.hasParameters
      flat: true
      maxValue: 1
      minValue: 0
      size: 28
      value: root.hasParameters ? root.rateParameter.baseValue : 0
      visible: !root.tempoSynced
      zero: 0

      onDraggingChanged: {
        if (root.hasParameters) {
          if (dragging)
            root.rateParameter.beginUserGesture();
          else
            root.rateParameter.endUserGesture();
        }
        root.editActiveChanged(dragging);
      }
      onValueModified: newValue => {
        if (root.hasParameters)
          root.parameterOperator.setValue(root.rateParameter, newValue);
      }
    }

    Label {
      Layout.fillWidth: true
      color: ZrythmTheme.placeholderTextColor
      elide: Text.ElideRight
      font: ZrythmTheme.xSmallTextFont
      horizontalAlignment: Text.AlignRight
      text: root.hasParameters && !root.tempoSynced
        ? root.rateParameter.range.convertFrom0To1(
            root.rateParameter.baseValue).toFixed(2) + " Hz" : ""
      verticalAlignment: Text.AlignVCenter
    }
  }
}
