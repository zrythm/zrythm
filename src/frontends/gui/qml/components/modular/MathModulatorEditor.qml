// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmActions
import ZrythmStyle

/**
 * Compact editor for the native Math modulator node.
 *
 * The host node supplies the operation parameter, resolved through the
 * engine's stable parameter ID, so edits go through the normal user-edit
 * APIs (undo, automation and MIDI mapping included).
 */
Item {
  id: root

  required property var operationParameter
  required property ProcessorParameterOperator parameterOperator

  signal editActiveChanged(bool active)

  readonly property bool hasParameter: operationParameter !== null
    && operationParameter !== undefined

  function syncCurrentIndex(): void {
    operationComboBox.currentIndex = root.hasParameter
      ? root.operationParameter.range.enumIndex(
          root.operationParameter.baseValue) : -1;
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 12
    anchors.rightMargin: 12
    spacing: 8

    Label {
      color: ZrythmTheme.textColor
      elide: Text.ElideRight
      font: ZrythmTheme.smallTextFont
      text: qsTr("A op B")
    }

    ComboBox {
      id: operationComboBox

      Layout.fillWidth: true
      font: ZrythmTheme.smallTextFont
      model: {
        if (!root.hasParameter)
          return [];
        const labels = [];
        for (let i = 0; i < root.operationParameter.range.enumCount(); ++i) {
          labels.push(root.operationParameter.range.enumLabel(i));
        }
        return labels;
      }

      Component.onCompleted: root.syncCurrentIndex()
      onActivated: function(index: int): void {
        if (root.hasParameter) {
          root.parameterOperator.setValue(
            root.operationParameter,
            root.operationParameter.range.normalizedEnumValue(index));
        }
      }

      Connections {
        function onBaseValueChanged(): void {
          root.syncCurrentIndex();
        }

        target: root.hasParameter ? root.operationParameter : null
      }
    }
  }
}
