// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

RegionBaseView {
  id: root

  property bool isLanePart: false
  required property TrackLane lane
  readonly property MidiRegion region: arrangerObject as MidiRegion

  regionContent: MidiRegionContent {
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    region: root.region
  }
}
