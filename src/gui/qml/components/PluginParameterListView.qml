// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

ListView {
  id: root

  property int maximumContentHeight: 240
  required property Plugin plugin

  clip: true
  implicitHeight: Math.min(contentHeight, maximumContentHeight)
  interactive: contentHeight > height
  spacing: 2

  // The model emits entries sorted by group (ungrouped first), so sections
  // are contiguous; ungrouped entries get no header
  section.property: "paramGroup"

  ScrollBar.vertical: ScrollBar {
    policy: root.interactive ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
  }
  section.delegate: Item {
    id: sectionDelegate

    required property string section

    height: section.length > 0 ? sectionLabel.implicitHeight + 10 : 0
    visible: section.length > 0
    width: root.width

    Label {
      id: sectionLabel

      anchors.left: parent.left
      anchors.leftMargin: 8
      anchors.right: parent.right
      anchors.rightMargin: 8
      anchors.verticalCenter: parent.verticalCenter
      elide: Text.ElideRight
      font: ZrythmTheme.semiBoldTextFont
      opacity: 0.8
      text: sectionDelegate.section
    }
  }
  delegate: RowLayout {
    id: paramDelegate

    required property int paramType
    required property ProcessorParameter parameter

    spacing: 4
    width: root.width

    Label {
      Layout.preferredWidth: 120
      elide: Text.ElideRight
      text: paramDelegate.parameter.label
    }

    // Toggle type (ParameterRange.Type.Toggle = 1)
    Loader {
      Layout.fillWidth: true
      active: paramDelegate.paramType === 1
      visible: active

      sourceComponent: Switch {
        checked: paramDelegate.parameter.baseValue >= 0.5

        onToggled: {
          paramDelegate.parameter.setBaseValueByUser(checked ? 1.0 : 0.0);
        }
      }
    }

    // Trigger type (ParameterRange.Type.Trigger = 6)
    Loader {
      Layout.fillWidth: true
      active: paramDelegate.paramType === 6
      visible: active

      sourceComponent: Button {
        text: qsTr("Trigger")

        onClicked: {
          paramDelegate.parameter.setBaseValueByUser(1.0);
        }
      }
    }

    // Slider types: Linear(0), Integer(2), GainAmplitude(3), Logarithmic(4), Enumeration(5)
    Loader {
      Layout.fillWidth: true
      active: paramDelegate.paramType === 0 || paramDelegate.paramType === 2 || paramDelegate.paramType === 3 || paramDelegate.paramType === 4 || paramDelegate.paramType === 5
      visible: active

      sourceComponent: RowLayout {
        Slider {
          Layout.fillWidth: true
          from: 0.0
          to: 1.0
          value: paramDelegate.parameter.baseValue

          onMoved: {
            paramDelegate.parameter.setBaseValueByUser(value);
          }
          onPressedChanged: {
            if (pressed) {
              paramDelegate.parameter.beginUserGesture();
            } else {
              paramDelegate.parameter.endUserGesture();
            }
          }
        }

        Label {
          text: {
            const realVal = paramDelegate.parameter.range.convertFrom0To1(paramDelegate.parameter.baseValue);
            let formatted;
            if (paramDelegate.paramType === 2) {
              formatted = Math.round(realVal).toString();
            } else {
              formatted = Number(realVal).toFixed(2);
            }
            return formatted;
          }
        }
      }
    }
  }
  model: PluginParameterListModel {
    plugin: root.plugin
  }
}
