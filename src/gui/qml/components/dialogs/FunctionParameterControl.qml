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

  Label {
    text: root.parameter.label
  }

  SpinBox {
    id: integerSpinBox

    visible: root.parameter.type === EditorFunctionParameter.Integer
    from: Math.round(root.parameter.minimum)
    to: Math.round(root.parameter.maximum)
    value: Math.round(root._initial)

    onValueModified: root.valueEdited(value)
  }

  DoubleSpinBox {
    id: doubleSpinBox

    visible: root.parameter.type === EditorFunctionParameter.Double
    decimals: 2
    from: root.parameter.minimum
    stepSize: root.parameter.maximum > root.parameter.minimum ? (root.parameter.maximum - root.parameter.minimum) / 100.0 : 0.1
    to: root.parameter.maximum
    value: root._initial

    onValueModified: root.valueEdited(value)
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
    currentIndex: indexOfValue(root._initial)

    onActivated: function(index) {
      root.valueEdited(currentValue);
    }
  }
}
