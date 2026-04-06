// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui

Item {
  id: root

  required property int contentHeight
  required property int contentWidth
  required property AudioRegion region

  AudioRegionWaveformCanvas {
    height: root.contentHeight
    outlineColor: Qt.lighter(Style.regionContentColor, 1.4)
    region: root.region
    waveformColor: Style.regionContentColor
    width: root.contentWidth
  }
}
