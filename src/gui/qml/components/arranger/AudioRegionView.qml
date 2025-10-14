// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

RegionBaseView {
  id: root

  property bool isLanePart: false
  required property TrackLane lane
  readonly property AudioRegion region: arrangerObject as AudioRegion

  regionContent: AudioRegionContent {
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    region: root.region
  }
}
