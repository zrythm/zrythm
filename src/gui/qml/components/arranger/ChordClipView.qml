// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

ClipBaseView {
  id: root

  readonly property ChordClip chordClip: arrangerObject as ChordClip

  clipContent: ChordClipContent {
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    chordClip: root.chordClip
  }
}
