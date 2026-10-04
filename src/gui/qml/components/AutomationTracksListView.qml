// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ListView {
  id: control

  property Track track

  boundsBehavior: Flickable.StopAtBounds
  clip: true
  implicitHeight: contentHeight
  interactive: false

  delegate: ItemDelegate {
    id: automationTrackItem

    readonly property AutomationTrack automationTrack: automationTrackHolder.automationTrack
    required property AutomationTrackHolder automationTrackHolder

    height: automationTrackHolder.height
    width: ListView.view.width

    Component.onCompleted: {
      console.log(automationTrackHolder, automationTrackHolder.height, height);
    }

    ButtonGroup {
      id: automationModeGroup

      exclusive: true
    }

    ColumnLayout {
      id: automationColumnLayout

      spacing: 4

      anchors {
        bottomMargin: 6
        fill: parent
        topMargin: 1
      }

      RowLayout {
        id: automationTopRowLayout

        Layout.alignment: Qt.AlignTop
        Layout.fillHeight: true
        Layout.fillWidth: true
        spacing: 6

        Button {
          Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
          Layout.fillHeight: false
          Layout.fillWidth: true
          Layout.preferredHeight: ZrythmTheme.compactControlHeight
          font: ZrythmTheme.smallTextFont
          icon.source: ResourceManager.getIconUrl("zrythm-dark", "automation-4p.svg")
          text: automationTrackItem.automationTrackHolder.automationTrack.parameter.label

          Component.onCompleted: {
            if (background && background instanceof Rectangle)
              background.radius = ZrythmTheme.textFieldRadius;
          }

          ToolTip {
            text: qsTr("Change automatable")
          }
        }

        Label {
          id: currentValueLabel

          Layout.alignment: Qt.AlignRight | Qt.AlignBaseline
          Layout.fillHeight: false
          Layout.fillWidth: false
          font: ZrythmTheme.smallTextFont
          text: ""

          Timer {
            interval: 50
            repeat: true
            running: automationTrackItem.visible
            triggeredOnStart: true

            onTriggered: {
              currentValueLabel.text = "%1%".arg(Math.round(automationTrackItem.automationTrackHolder.automationTrack.parameter.currentValue() * 100));
            }
          }
        }
      }

      RowLayout {
        id: automationBottomRowLayout

        Layout.alignment: Qt.AlignBottom
        Layout.fillHeight: false
        Layout.fillWidth: true
        visible: automationTrackItem.automationTrackHolder.height > automationTopRowLayout.height + height + 6

        LinkedButtons {
          id: automationModeButtons

          Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
          Layout.fillHeight: false
          Layout.fillWidth: false

          Button {
            ButtonGroup.group: automationModeGroup
            checkable: true
            checked: automationTrackItem.automationTrack.automationMode === 0
            font: ZrythmTheme.channelStateTextFont
            Layout.preferredHeight: ZrythmTheme.compactControlHeight
            text: qsTr("On")

            onClicked: {
              automationTrackItem.automationTrack.automationMode = 0;
            }
          }

          Button {
            ButtonGroup.group: automationModeGroup
            checkable: true
            checked: automationTrackItem.automationTrack.automationMode === 1
            font: ZrythmTheme.channelStateTextFont
            Layout.preferredHeight: ZrythmTheme.compactControlHeight
            text: automationTrackItem.automationTrack.recordMode === 0 ? qsTr("Touch") : qsTr("Latch")

            onClicked: {
              if (automationTrackItem.automationTrack.automationMode === 1)
                automationTrackItem.automationTrack.recordMode = automationTrackItem.automationTrack.recordMode === 0 ? 1 : 0;

              automationTrackItem.automationTrack.automationMode = 1;
            }
          }

          Button {
            ButtonGroup.group: automationModeGroup
            checkable: true
            checked: automationTrackItem.automationTrack.automationMode === 2
            font: ZrythmTheme.channelStateTextFont
            Layout.preferredHeight: ZrythmTheme.compactControlHeight
            text: qsTr("Off")

            onClicked: {
              automationTrackItem.automationTrack.automationMode = 2;
            }
          }
        }

        Item {
          id: bottomAutomationSpacer

          Layout.fillHeight: false
          Layout.fillWidth: true
        }

        LinkedButtons {
          Layout.alignment: Qt.AlignRight | Qt.AlignBaseline
          Layout.fillHeight: false
          Layout.fillWidth: false

          Button {
            id: removeAutomationTrackButton

            Layout.preferredHeight: ZrythmTheme.compactControlHeight
            padding: 3

            // icon.source: ResourceManager.getIconUrl("zrythm-dark", "remove.svg")
            text: "-"

            onClicked: {
              control.track.automationTracklist.hideAutomationTrack(automationTrackItem.automationTrack);
            }

            ToolTip {
              text: qsTr("Hide automation lane")
            }

            font {
              bold: true
              family: ZrythmTheme.fontFamily
              pixelSize: 12
            }
          }

          Button {
            id: addAutomationTrackButton

            Layout.preferredHeight: ZrythmTheme.compactControlHeight
            padding: 3

            // icon.source: ResourceManager.getIconUrl("zrythm-dark", "add.svg")
            text: "+"

            onClicked: {
              control.track.automationTracklist.showNextAvailableAutomationTrack(automationTrackItem.automationTrack);
            }

            ToolTip {
              text: qsTr("Add automation lane")
            }

            font {
              bold: true
              family: ZrythmTheme.fontFamily
              pixelSize: 12
            }
          }
        }
      }
    }

    ResizeHandle {
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      resizeTarget: automationTrackItem.automationTrackHolder
    }
  }
  model: SortFilterProxyModel {
    id: proxyModel

    model: control.track.automationTracklist

    filters: [
      FunctionFilter {
        function filter(data: ATHRoleData): bool {
          return data.automationTrackHolder.createdByUser && data.automationTrackHolder.visible;
        }
      }
    ]
  }

  component ATHRoleData: QtObject {
    property AutomationTrackHolder automationTrackHolder
  }
}
