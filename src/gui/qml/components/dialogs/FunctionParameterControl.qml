// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

/**
 * Label and editor control for one editor-function parameter.
 *
 * Exactly one control is visible, chosen by the parameter type. The current
 * value is reported through valueEdited whenever the user changes it.
 */
RowLayout {
  id: root

  required property EditorFunctionParameter parameter

  /// Prefilled value; falls back to the parameter's default when unset.
  property var initialValue: undefined

  signal valueEdited(var value)

  readonly property var _initial: root.initialValue !== undefined ? root.initialValue : root.parameter.defaultValue
  readonly property bool _isInteger: root.parameter.type === EditorFunctionParameter.Integer

  Label {
    text: root.parameter.label
  }

  EditableSlider {
    id: numericSlider

    visible: root._isInteger || root.parameter.type === EditorFunctionParameter.Double
    Layout.fillWidth: true
    decimals: root._isInteger || root.parameter.step >= 1 ? 0 : 2
    from: root.parameter.minimum
    to: root.parameter.maximum
    stepSize: root.parameter.step
    value: root._initial

    onValueEdited: function(value) {
      root.valueEdited(value);
    }
  }

  CheckBox {
    id: boolCheckBox

    visible: root.parameter.type === EditorFunctionParameter.Boolean
    checked: root._initial

    onToggled: root.valueEdited(checked)
  }

  ComboBox {
    id: choiceComboBox

    visible: root.parameter.type === EditorFunctionParameter.Choice
    model: root.parameter.choices
    textRole: "display"
    valueRole: "value"
    currentIndex: count > 0 ? indexOfValue(root._initial) : -1

    onActivated: function(index) {
      root.valueEdited(currentValue);
    }
  }
}
