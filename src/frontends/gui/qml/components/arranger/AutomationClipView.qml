// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

ClipBaseView {
  id: root

  readonly property AutomationClip automationClip: arrangerObject as AutomationClip
  required property AutomationTrack automationTrack

  clipContent: AutomationClipContent {
    automationClip: root.automationClip
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    inkColor: root.contentInkColor
    loopPreview: root.loopPreview
    referenceWidth: root.referenceWidth
    referenceX: root.referenceX
  }
}
