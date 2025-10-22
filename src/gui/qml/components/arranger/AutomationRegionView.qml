// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

RegionBaseView {
  id: root

  required property AutomationTrack automationTrack
  readonly property AutomationRegion region: arrangerObject as AutomationRegion

  regionContent: AutomationRegionContent {
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    region: root.region
  }
}
