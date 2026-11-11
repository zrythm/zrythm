// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

/**
 * Channel sends section shared by the mixer strip and the track
 * inspector: an expandable panel with an add action in the header and
 * one row per send.
 */
ExpanderBox {
  id: root

  required property ChannelSendRouting channelSendRouting
  property string headerIconName: "audio-send.svg"
  required property ProcessorParameterOperator parameterOperator
  required property Project project
  required property ChannelSendGroup sends
  required property Track track
  required property UndoStack undoStack

  icon.source: root.headerIconName !== "" ? ResourceManager.getIconUrl("zrythm-dark", root.headerIconName) : ""

  headerAction: Component {
    Button {
      id: panelAddSendButton

      Accessible.name: qsTr("Add send")
      flat: true
      icon.color: palette.buttonText
      icon.height: 12
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "add.svg")
      icon.width: 12
      implicitHeight: 18
      implicitWidth: 18
      padding: 0

      background: Rectangle {
        color: panelAddSendButton.hovered || panelAddSendButton.visualFocus || panelAddSendButton.down ? ZrythmTheme.buttonHoverBackgroundAppendColor : "transparent"
        radius: ZrythmTheme.textFieldRadius
      }

      onClicked: panelAddSendOp.addSend(false)

      ToolTip {
        text: qsTr("Add a send")
      }
    }
  }

  frameContentItem: ColumnLayout {
    spacing: 4

    TrackOperator {
      id: panelAddSendOp

      channelSendRouting: root.channelSendRouting
      project: root.project
      track: root.track
      undoStack: root.undoStack
    }

    Repeater {
      model: root.sends

      delegate: ChannelSendControls {
        Layout.fillWidth: true
        channelSendRouting: root.channelSendRouting
        parameterOperator: root.parameterOperator
        project: root.project
        track: root.track
        undoStack: root.undoStack
      }
    }
  }
}
