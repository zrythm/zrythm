// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui

Item {
  id: root

  required property int contentHeight
  required property int contentWidth
  required property MidiRegion region

  MidiRegionCanvas {
    height: root.contentHeight
    noteColor: Style.regionContentColor
    region: root.region
    width: root.contentWidth
  }
}
