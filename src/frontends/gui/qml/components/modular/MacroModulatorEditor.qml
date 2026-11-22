// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmActions
import ZrythmStyle

/**
 * Compact editor for the native Macro modulator node.
 *
 * The host node supplies the parameter object, resolved through the macro
 * value parameter's stable ID, so edits go through the normal user-edit
 * APIs (undo, automation and MIDI mapping included).
 */
Item {
  id: root

  required property var parameter
  required property ProcessorParameterOperator parameterOperator

  signal editActiveChanged(bool active)

  readonly property bool hasParameter: parameter !== null && parameter !== undefined

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 12
    anchors.rightMargin: 12
    spacing: 10

    Label {
      Layout.fillWidth: true
      color: ZrythmTheme.textColor
      elide: Text.ElideRight
      font: ZrythmTheme.smallTextFont
      text: qsTr("Macro")
    }

    Knob {
      id: valueKnob

      defaultValue: root.hasParameter ? root.parameter.range.defaultNormalizedValue : 0
      enabled: root.hasParameter
      flat: true
      maxValue: 1
      minValue: 0
      size: 28
      value: root.hasParameter ? root.parameter.baseValue : 0
      zero: 0

      onDraggingChanged: {
        if (root.hasParameter) {
          if (dragging)
            root.parameter.beginUserGesture();
          else
            root.parameter.endUserGesture();
        }
        root.editActiveChanged(dragging);
      }
      onValueModified: newValue => {
        if (root.hasParameter)
          root.parameterOperator.setValue(root.parameter, newValue);
      }
    }

    Label {
      color: ZrythmTheme.placeholderTextColor
      font: ZrythmTheme.xSmallTextFont
      horizontalAlignment: Text.AlignRight
      text: root.hasParameter ? Math.round(root.parameter.baseValue * 100) + "%" : ""
      verticalAlignment: Text.AlignVCenter
    }
  }
}
