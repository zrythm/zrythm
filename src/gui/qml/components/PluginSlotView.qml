// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

Control {
  id: root

  property bool down: false
  property bool openPluginInspectorOnClick: false
  required property Plugin plugin
  readonly property bool pluginEnabled: root.plugin && root.plugin.bypassParameter.baseValue < 0.5
  property bool selected: false
  required property Track track

  signal pluginDeselected(var plugin)
  signal pluginDragStarted(var plugin)
  signal pluginSelected(var plugin, bool ctrlPressed)

  implicitHeight: 20
  implicitWidth: 48
  width: parent ? parent.width : 200

  // Context menu
  ContextMenu.menu: Menu {
    id: contextMenu

    MenuItem {
      enabled: root.plugin !== null
      text: root.plugin ? qsTr("Remove Plugin") : qsTr("Add Plugin")

      onTriggered: {
        if (root.plugin && root.track && root.track.channel) {
          console.log("remove plugin");
          // root.track.channel.removePlugin(root.slotIndex, root.slotType);
        }
      }
    }

    MenuItem {
      enabled: root.plugin !== null
      text: qsTr("Show Plugin UI")

      onTriggered: {
        if (root.plugin) {
          root.plugin.uiVisible = true;
        }
      }
    }

    MenuSeparator {
      visible: root.plugin !== null
    }

    MenuItem {
      text: qsTr("Properties")

      onTriggered:
      // Open plugin properties dialog
      {}
    }
  }

  // Mouse handling
  MouseArea {
    id: mouseArea

    anchors.fill: parent
    hoverEnabled: true

    onClicked: function (mouse) {
      if (mouse.button === Qt.LeftButton) {
        if (root.plugin) {
          if (mouse.modifiers & Qt.ControlModifier) {
            if (root.selected) {
              root.pluginDeselected(root.plugin);
            } else {
              root.pluginSelected(root.plugin, true);
            }
          } else {
            root.pluginSelected(root.plugin, false);
          }
        }
      } else if (mouse.button === Qt.RightButton) {
        contextMenu.popup();
      }
    }
    onDoubleClicked: {
      if (root.plugin) {
        root.plugin.uiVisible = !root.plugin.uiVisible;
      }
    }
    // onEntered: root.hovered = true
    // onExited: root.hovered = false
    onPressAndHold: {
      if (root.plugin) {
        root.pluginDragStarted(root.plugin);
      }
    }
    onPressed: root.down = true
    onReleased: root.down = false
  }

  // Drop handling
  DropArea {
    id: dropArea

    anchors.fill: parent

    onDropped: {
      console.log("dropped");
    }
    onEntered: {
      if (drag.source && drag.source.hasOwnProperty('pluginDescriptor')) {
        bgRect.border.color = palette.highlight;
        bgRect.border.width = 2;
      }
    }
    onExited: {
      bgRect.border.color = root.selected ? palette.highlight : palette.dark;
      bgRect.border.width = 1;
    }
  }

  Rectangle {
    id: bgRect

    readonly property color baseColor: {
      let c = root.pluginEnabled ? palette.base : palette.window;
      if (root.plugin.uiVisible) {
        c = Style.getColorBlendedTowardsContrast(c);
      }
      return c;
    }

    anchors.fill: parent
    border.color: root.selected ? palette.highlight : palette.alternateBase
    border.width: 1
    color: Style.adjustColorForHoverOrVisualFocusOrDown(baseColor, root.hovered, root.visualFocus, root.down)
    radius: 6
  }

  RowLayout {
    anchors.fill: parent
    spacing: 4

    // Activate button
    RoundButton {
      id: activateButton

      Layout.alignment: Qt.AlignVCenter
      Layout.leftMargin: 4
      Layout.preferredHeight: 8
      Layout.preferredWidth: 8
      checkable: true
      checked: root.pluginEnabled
      radius: 16

      onClicked: {
        if (root.plugin) {
          root.plugin.bypassParameter.baseValue = (root.pluginEnabled) ? 1.0 : 0.0;
        }
      }
    }

    // Text display
    Text {
      id: slotText

      Layout.alignment: Qt.AlignVCenter
      Layout.fillWidth: true
      Layout.rightMargin: 4
      color: {
        let c;
        if (root.plugin && root.plugin.instantiationStatus === Plugin.Failed) {
          c = Style.dangerColor;
        } else {
          c = palette.text;
        }
        if (!root.pluginEnabled) {
          return Style.getColorBlendedTowardsContrast(c);
        }
        return c;
      }
      elide: Text.ElideRight
      text: {
        if (root.plugin) {
          if (root.plugin.instantiationStatus === Plugin.Failed) {
            return "(!) " + root.plugin.configuration.descriptor.name;
          } else {
            return root.plugin.configuration.descriptor.name;
          }
        } else {
          return "";
        }
      }
    }
  }

  ToolTip {
    text: root.plugin ? root.plugin.configuration.descriptor.name : ""
  }
}
