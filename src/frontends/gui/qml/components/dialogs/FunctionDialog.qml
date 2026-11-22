// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

/**
 * Generic parameter dialog for editor functions.
 *
 * Renders one control per parameter of the function definition. Parameters
 * the user did not touch keep the value given by initialValues (falling
 * back to each parameter's default). The collected values are emitted as a
 * {key: value} object through optionsConfirmed when the dialog is accepted.
 */
Dialog {
  id: root

  required property EditorFunctionDefinition functionDefinition

  /// Parameter values to prefill the controls with, keyed by parameter key.
  property var initialValues: ({})

  signal optionsConfirmed(var options)

  // Parameter values the user changed, keyed by parameter key. Only mutated
  // imperatively by the delegates and read on accept.
  property var _editedValues: ({})

  title: qsTr("Apply %1").arg(root.functionDefinition.label)
  modal: true
  popupType: Popup.Window
  standardButtons: Dialog.Ok | Dialog.Cancel

  onAccepted: {
    const options = {};
    for (const param of root.functionDefinition.parameters) {
      if (param.key in root._editedValues)
        options[param.key] = root._editedValues[param.key];
      else if (param.key in root.initialValues)
        options[param.key] = root.initialValues[param.key];
      else
        options[param.key] = param.defaultValue;
    }
    root.optionsConfirmed(options);
  }

  contentItem: ColumnLayout {
    spacing: ZrythmTheme.buttonPadding

    Repeater {
      id: parametersRepeater

      model: root.functionDefinition.parameters

      FunctionParameterControl {
        required property var modelData

        Layout.fillWidth: true
        parameter: modelData
        initialValue: modelData.key in root.initialValues ? root.initialValues[modelData.key] : undefined

        onValueEdited: function(value) {
          root._editedValues[modelData.key] = value;
        }
      }
    }
  }
}
