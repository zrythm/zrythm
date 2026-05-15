// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Qt.labs.synchronizer
import Zrythm
import ZrythmControllers
import ZrythmStyle

SplitButton {
  id: root

  readonly property int _currentMode: root.appSettings.recordingMode
  required property AppSettings appSettings
  required property Transport transport

  iconSource: ResourceManager.getIconUrl("zrythm-dark", "record.svg")
  mainButton.checkable: true
  mainButton.palette.accent: Style.dangerColor
  mainButton.palette.buttonText: Style.dangerColor
  menuTooltipText: qsTr("Record Options")
  tooltipText: qsTr("Record")

  Synchronizer on mainButton.checked {
    sourceObject: root.transport
    sourceProperty: "recordEnabled"
  }

  menuItems: Menu {
    id: recordMenu

    Action {
      checkable: true
      checked: root._currentMode === TransportController.RecordingMode.Takes
      text: qsTr("Create takes")

      onTriggered: root.appSettings.recordingMode = TransportController.RecordingMode.Takes
    }

    Action {
      checkable: true
      checked: root._currentMode === TransportController.RecordingMode.TakesMuted
      text: qsTr("Create takes (mute previous)")

      onTriggered: root.appSettings.recordingMode = TransportController.RecordingMode.TakesMuted
    }

    MenuSeparator {
    }

    MenuItem {
      checkable: true
      checked: root.transport.punchEnabled
      text: qsTr("Punch in/out")

      onTriggered: root.transport.punchEnabled = !root.transport.punchEnabled
    }
  }
}
