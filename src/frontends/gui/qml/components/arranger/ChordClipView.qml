// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmStyle

ClipBaseView {
  id: root

  readonly property ChordClip chordClip: arrangerObject as ChordClip

  clipContent: ChordClipContent {
    chordClip: root.chordClip
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    textColor: ZrythmTheme.polarityTextColor(root.clipBodyColor)
    loopPreview: root.loopPreview
    referenceWidth: root.referenceWidth
    referenceX: root.referenceX
  }
}
