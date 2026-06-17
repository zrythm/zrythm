// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

RegionBaseView {
  id: root

  readonly property ChordRegion region: arrangerObject as ChordRegion

  regionContent: ChordRegionContent {
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    region: root.region
  }
}
