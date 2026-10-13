// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle

/**
 * Slider with a value readout.
 *
 * The value changes by dragging the slider or by double clicking the
 * value readout, which opens a small dialog for typing an exact value.
 * Every user change is reported through valueEdited.
 */
RowLayout {
  id: root

  /// Inclusive value bounds.
  property real from: 0
  property real to: 100

  /// Value increment the slider snaps to.
  property real stepSize: 1

  property real value: from

  /// Digits after the decimal point of the readout and the dialog input;
  /// 0 restricts the value to whole numbers.
  property int decimals: 0

  signal valueEdited(real value)

  readonly property string _formatted: decimals > 0 ? value.toFixed(decimals) : Math.round(value).toString()

  Slider {
    id: slider

    Layout.fillWidth: true
    from: root.from
    to: root.to
    value: root.value
    stepSize: root.stepSize

    onMoved: {
      root.value = value;
      root.valueEdited(value);
    }
  }

  Label {
    id: readoutLabel

    objectName: "readoutLabel"
    text: root._formatted

    MouseArea {
      anchors.fill: parent

      onDoubleClicked: editDialog.open()
    }
  }

  Dialog {
    id: editDialog

    objectName: "editDialog"
    title: qsTr("Edit Value")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    onAboutToShow: {
      // the footer is only complete once the dialog opens, so gate the OK
      // button here rather than in Component.onCompleted
      standardButton(Dialog.Ok).enabled = Qt.binding(() => valueField.acceptableInput);
      valueField.text = root._formatted;
      valueField.selectAll();
      valueField.forceActiveFocus();
    }
    onAccepted: {
      const parsed = parseFloat(valueField.text);
      root.value = parsed;
      root.valueEdited(parsed);
    }

    TextField {
      id: valueField

      objectName: "valueField"
      validator: root.decimals > 0 ? doubleValidator : integerValidator
    }
  }

  DoubleValidator {
    id: doubleValidator

    bottom: root.from
    decimals: root.decimals
    notation: DoubleValidator.StandardNotation
    top: root.to
  }

  IntValidator {
    id: integerValidator

    bottom: Math.round(root.from)
    top: Math.round(root.to)
  }
}
