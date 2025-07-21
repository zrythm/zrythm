// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

RowLayout {
  id: root

  required property var tool

  ButtonGroup {
    id: toolGroup

  }

  LinkedButtons {
    id: linkedButtons

    Layout.fillHeight: true
    Layout.fillWidth: true

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === Tool.Select
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "box-dotted-symbolic.svg")

      onClicked: {
        root.tool.toolValue = Tool.Select;
      }

      ToolTip {
        text: qsTr("Select tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === Tool.Edit
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "pencil-symbolic.svg")

      onClicked: {
        root.tool.toolValue = Tool.Edit;
      }

      ToolTip {
        text: qsTr("Pencil tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === Tool.Cut
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "cut-symbolic.svg")

      onClicked: {
        root.tool.toolValue = Tool.Cut;
      }

      ToolTip {
        text: qsTr("Scissors tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === Tool.Eraser
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "eraser2-symbolic.svg")

      onClicked: {
        root.tool.toolValue = Tool.Eraser;
      }

      ToolTip {
        text: qsTr("Eraser tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === Tool.Ramp
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "draw-line.svg")

      onClicked: {
        root.tool.toolValue = Tool.Ramp;
      }

      ToolTip {
        text: qsTr("Ramp tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === Tool.Audition
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "audio-volume-high.svg")

      onClicked: {
        root.tool.toolValue = Tool.Audition;
      }

      ToolTip {
        text: qsTr("Audition tool")
      }
    }
  }
}
