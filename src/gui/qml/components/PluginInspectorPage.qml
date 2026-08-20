// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

ScrollView {
  id: root

  required property Plugin plugin

  ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
  contentWidth: availableWidth
  implicitWidth: content.implicitWidth + (ScrollBar.vertical.visible ? ScrollBar.vertical.width : 0)

  ColumnLayout {
    id: content

    width: root.availableWidth

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
            root.plugin.bypassParameter.setBaseValueByUser(checked ? 0.0 : 1.0);
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
              root.plugin.gainParameter.setBaseValueByUser(value);
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

      frameContentItem: PluginParameterListView {
        plugin: root.plugin
      }
    }
  }
}
