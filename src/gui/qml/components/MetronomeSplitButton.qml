// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle
import Qt.labs.synchronizer

SplitButton {
  id: root

  required property Metronome metronome

  iconSource: ResourceManager.getIconUrl("gnome-icon-library", "metronome-symbolic.svg")
  mainButton.checkable: true
  menuTooltipText: qsTr("Metronome Options")
  tooltipText: qsTr("Metronome")

  Synchronizer on mainButton.checked {
    sourceObject: root.metronome
    sourceProperty: "enabled"
  }

  // TODO
  menuItems: Menu {
    id: menu

    MenuItem {
      text: "(coming soon)"
    }
  }
}
