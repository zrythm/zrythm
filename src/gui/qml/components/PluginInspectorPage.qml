// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  required property Plugin plugin

  PluginParameterListModel {
    id: pluginParameterModel

    plugin: root.plugin
  }

  // Plugin Properties
  ExpanderBox {
    Layout.fillWidth: true
    icon.source: ResourceManager.getIconUrl("gnome-icon-library", "general-properties-symbolic.svg")
    title: qsTr("Plugin Properties")

    frameContentItem: ColumnLayout {
      spacing: 4

      Label {
        text: qsTr("Name")
      }

      Label {
        Layout.fillWidth: true
        text: root.plugin.configuration.descriptor.name
        wrapMode: Text.Wrap
      }

      Label {
        text: qsTr("Category")
      }

      Label {
        Layout.fillWidth: true
        text: root.plugin.configuration.descriptor.category
      }

      Label {
        text: qsTr("Enabled")
      }

      Switch {
        checked: root.plugin.bypassParameter.baseValue < 0.5

        onToggled: {
          root.plugin.bypassParameter.baseValue = checked ? 0.0 : 1.0;
        }
      }

      Button {
        text: root.plugin.uiVisible ? qsTr("Hide UI") : qsTr("Show UI")

        onClicked: {
          root.plugin.uiVisible = !root.plugin.uiVisible;
        }
      }

      Label {
        text: qsTr("Gain")
      }

      RowLayout {
        Layout.fillWidth: true

        Slider {
          Layout.fillWidth: true
          from: 0.0
          to: 1.0
          value: root.plugin.gainParameter.baseValue

          onMoved: {
            root.plugin.gainParameter.baseValue = value;
          }
        }

        Label {
          text: {
            const val = root.plugin.gainParameter.range.convertFrom0To1(root.plugin.gainParameter.baseValue);
            return Number(val).toFixed(2);
          }
        }
      }
    }
  }

  // Parameters (scrollable)
  ExpanderBox {
    Layout.fillWidth: true
    title: qsTr("Parameters")

    frameContentItem: ListView {
      id: paramList

      readonly property int maxHeight: 240

      clip: true
      implicitHeight: Math.min(contentHeight, maxHeight)
      interactive: contentHeight > maxHeight
      model: pluginParameterModel
      spacing: 2

      ScrollBar.vertical: ScrollBar {
        policy: paramList.interactive ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
      }
      delegate: RowLayout {
        id: paramDelegate

        required property int paramType
        required property ProcessorParameter parameter

        spacing: 4
        width: paramList.width

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
              paramDelegate.parameter.baseValue = checked ? 1.0 : 0.0;
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
              paramDelegate.parameter.baseValue = 1.0;
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
                paramDelegate.parameter.baseValue = value;
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
    }
  }
}
