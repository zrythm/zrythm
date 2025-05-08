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

        Layout.fillWidth: true
        Layout.fillHeight: true

        Button {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "box-dotted-symbolic.svg")
            ButtonGroup.group: toolGroup
            checkable: true
            checked: root.tool.toolValue === Tool.Select
            onClicked: {
                root.tool.toolValue = Tool.Select;
            }

            ToolTip {
                text: qsTr("Select tool")
            }

        }

        Button {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "pencil-symbolic.svg")
            ButtonGroup.group: toolGroup
            checkable: true
            checked: root.tool.toolValue === Tool.Edit
            onClicked: {
                root.tool.toolValue = Tool.Edit;
            }

            ToolTip {
                text: qsTr("Pencil tool")
            }

        }

        Button {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "cut-symbolic.svg")
            ButtonGroup.group: toolGroup
            checkable: true
            checked: root.tool.toolValue === Tool.Cut
            onClicked: {
                root.tool.toolValue = Tool.Cut;
            }

            ToolTip {
                text: qsTr("Scissors tool")
            }

        }

        Button {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "eraser2-symbolic.svg")
            ButtonGroup.group: toolGroup
            checkable: true
            checked: root.tool.toolValue === Tool.Eraser
            onClicked: {
                root.tool.toolValue = Tool.Eraser;
            }

            ToolTip {
                text: qsTr("Eraser tool")
            }

        }

        Button {
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "draw-line.svg")
            ButtonGroup.group: toolGroup
            checkable: true
            checked: root.tool.toolValue === Tool.Ramp
            onClicked: {
                root.tool.toolValue = Tool.Ramp;
            }

            ToolTip {
                text: qsTr("Ramp tool")
            }

        }

        Button {
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "audio-volume-high.svg")
            ButtonGroup.group: toolGroup
            checkable: true
            checked: root.tool.toolValue === Tool.Audition
            onClicked: {
                root.tool.toolValue = Tool.Audition;
            }

            ToolTip {
                text: qsTr("Audition tool")
            }

        }

    }

}
