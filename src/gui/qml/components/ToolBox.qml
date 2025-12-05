// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

RowLayout {
  id: root

  required property ArrangerTool tool

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
      checked: root.tool.toolValue === ArrangerTool.Select
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "box-dotted-symbolic.svg")

      onClicked: {
        root.tool.toolValue = ArrangerTool.Select;
      }

      ToolTip {
        text: qsTr("Select tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === ArrangerTool.Edit
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "pencil-symbolic.svg")

      onClicked: {
        root.tool.toolValue = ArrangerTool.Edit;
      }

      ToolTip {
        text: qsTr("Pencil tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === ArrangerTool.Cut
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "cut-symbolic.svg")

      onClicked: {
        root.tool.toolValue = ArrangerTool.Cut;
      }

      ToolTip {
        text: qsTr("Scissors tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === ArrangerTool.Eraser
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "eraser2-symbolic.svg")

      onClicked: {
        root.tool.toolValue = ArrangerTool.Eraser;
      }

      ToolTip {
        text: qsTr("Eraser tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === ArrangerTool.Ramp
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "draw-line.svg")

      onClicked: {
        root.tool.toolValue = ArrangerTool.Ramp;
      }

      ToolTip {
        text: qsTr("Ramp tool")
      }
    }

    Button {
      ButtonGroup.group: toolGroup
      checkable: true
      checked: root.tool.toolValue === ArrangerTool.Audition
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "audio-volume-high.svg")

      onClicked: {
        root.tool.toolValue = ArrangerTool.Audition;
      }

      ToolTip {
        text: qsTr("Audition tool")
      }
    }
  }
}
