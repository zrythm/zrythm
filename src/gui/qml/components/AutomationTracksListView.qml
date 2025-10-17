// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ListView {
  id: control

  property var track

  clip: true
  implicitHeight: contentHeight
  interactive: false

  delegate: ItemDelegate {
    id: automationTrackItem

    readonly property var automationTrack: automationTrackHolder.automationTrack
    required property var automationTrackHolder

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

      readonly property font buttonFont: Style.semiBoldTextFont

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
          font: automationColumnLayout.buttonFont
          icon.source: ResourceManager.getIconUrl("zrythm-dark", "automation-4p.svg")
          padding: Style.buttonPadding
          styleHeight: Style.buttonHeight
          text: automationTrackItem.automationTrackHolder.automationTrack.parameter.label

          Component.onCompleted: {
            if (background && background instanceof Rectangle)
              background.radius = Style.textFieldRadius;
          }

          ToolTip {
            text: qsTr("Change automatable")
          }
        }

        Label {
          Layout.alignment: Qt.AlignRight | Qt.AlignBaseline
          Layout.fillHeight: false
          Layout.fillWidth: false
          font: Style.smallTextFont
          text: {
            automationTrackItem.automationTrackHolder.automationTrack.parameter.baseValue;
            return automationTrackItem.automationTrackHolder.automationTrack.parameter.range.convertFrom0To1(automationTrackItem.automationTrackHolder.automationTrack.parameter.currentValue());
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
          layer.enabled: true

          Button {
            ButtonGroup.group: automationModeGroup
            checkable: true
            checked: automationTrackItem.automationTrack.automationMode === 0
            font: automationColumnLayout.buttonFont
            padding: Style.buttonPadding
            styleHeight: Style.buttonHeight
            text: qsTr("On")

            onClicked: {
              automationTrackItem.automationTrack.automationMode = 0;
            }
          }

          Button {
            ButtonGroup.group: automationModeGroup
            checkable: true
            checked: automationTrackItem.automationTrack.automationMode === 1
            font: automationColumnLayout.buttonFont
            padding: Style.buttonPadding
            styleHeight: Style.buttonHeight
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
            font: automationColumnLayout.buttonFont
            padding: Style.buttonPadding
            styleHeight: Style.buttonHeight
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
          layer.enabled: true

          Button {
            id: removeAutomationTrackButton

            padding: Style.buttonPadding
            styleHeight: Style.buttonHeight

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
              family: Style.fontFamily
              pixelSize: 14
            }
          }

          Button {
            id: addAutomationTrackButton

            padding: Style.buttonPadding
            styleHeight: Style.buttonHeight

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
              family: Style.fontFamily
              pixelSize: 14
            }
          }
        }
      }
    }

    Loader {
      property var resizeTarget: automationTrackItem.automationTrackHolder

      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      sourceComponent: resizeHandle
    }

    Connections {
      function onHeightChanged() {
        control.track.fullVisibleHeightChanged();
      }

      target: automationTrackItem.automationTrackHolder
    }
  }
  Behavior on implicitHeight {
    animation: Style.propertyAnimation
  }
  model: AutomationTracklistProxyModel {
    showOnlyCreated: true
    showOnlyVisible: true
    sourceModel: control.track.automationTracklist
  }

  Connections {
    function onDataChanged() {
      control.track.fullVisibleHeightChanged();
    }

    function onModelReset() {
      control.track.fullVisibleHeightChanged();
    }

    function onRowsInserted() {
      control.track.fullVisibleHeightChanged();
    }

    function onRowsRemoved() {
      control.track.fullVisibleHeightChanged();
    }

    target: control.model
  }
}
