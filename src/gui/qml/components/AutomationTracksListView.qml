// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle 1.0

ListView {
  id: automationTracksListView

  property var track

  clip: true
  implicitHeight: contentHeight
  interactive: false

  delegate: ItemDelegate {
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
        topMargin: control.contentTopMargins
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
          padding: control.buttonPadding
          styleHeight: control.buttonHeight
          text: automationTrackHolder.automationTrack.parameter.label

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
            automationTrackHolder.automationTrack.parameter.baseValue;
            return automationTrackHolder.automationTrack.parameter.range.convertFrom0To1(automationTrackHolder.automationTrack.parameter.currentValue());
          }
        }
      }

      RowLayout {
        id: automationBottomRowLayout

        Layout.alignment: Qt.AlignBottom
        Layout.fillHeight: false
        Layout.fillWidth: true
        visible: automationTrackHolder.height > automationTopRowLayout.height + height + 6

        LinkedButtons {
          id: automationModeButtons

          Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
          Layout.fillHeight: false
          Layout.fillWidth: false
          layer.enabled: true

          Button {
            ButtonGroup.group: automationModeGroup
            checkable: true
            checked: automationTrack.automationMode === 0
            font: automationColumnLayout.buttonFont
            padding: control.buttonPadding
            styleHeight: control.buttonHeight
            text: qsTr("On")

            onClicked: {
              automationTrack.automationMode = 0;
            }
          }

          Button {
            ButtonGroup.group: automationModeGroup
            checkable: true
            checked: automationTrack.automationMode === 1
            font: automationColumnLayout.buttonFont
            padding: control.buttonPadding
            styleHeight: control.buttonHeight
            text: automationTrack.recordMode === 0 ? qsTr("Touch") : qsTr("Latch")

            onClicked: {
              if (automationTrack.automationMode === 1)
                automationTrack.recordMode = automationTrack.recordMode === 0 ? 1 : 0;

              automationTrack.automationMode = 1;
            }
          }

          Button {
            ButtonGroup.group: automationModeGroup
            checkable: true
            checked: automationTrack.automationMode === 2
            font: automationColumnLayout.buttonFont
            padding: control.buttonPadding
            styleHeight: control.buttonHeight
            text: qsTr("Off")

            onClicked: {
              automationTrack.automationMode = 2;
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

            padding: control.buttonPadding
            styleHeight: control.buttonHeight

            // icon.source: ResourceManager.getIconUrl("zrythm-dark", "remove.svg")
            text: "-"

            onClicked: {
              track.automatableTrackMixin.automationTracklist.hideAutomationTrack(automationTrack);
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

            padding: control.buttonPadding
            styleHeight: control.buttonHeight

            // icon.source: ResourceManager.getIconUrl("zrythm-dark", "add.svg")
            text: "+"

            onClicked: {
              track.automatableTrackMixin.automationTracklist.showNextAvailableAutomationTrack(automationTrack);
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
      property var resizeTarget: automationTrackHolder

      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      sourceComponent: resizeHandle
    }

    Connections {
      function onHeightChanged() {
        track.fullVisibleHeightChanged();
      }

      target: automationTrackHolder
    }
  }
  Behavior on implicitHeight {
    animation: Style.propertyAnimation
  }
  model: AutomationTracklistProxyModel {
    showOnlyCreated: true
    showOnlyVisible: true
    sourceModel: track.automatableTrackMixin.automationTracklist
  }

  Connections {
    function onDataChanged() {
      track.fullVisibleHeightChanged();
    }

    function onModelReset() {
      track.fullVisibleHeightChanged();
    }

    function onRowsInserted() {
      track.fullVisibleHeightChanged();
    }

    function onRowsRemoved() {
      track.fullVisibleHeightChanged();
    }

    target: automationTracksListView.model
  }
}
