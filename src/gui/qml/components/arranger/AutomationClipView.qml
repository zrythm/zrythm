// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

ClipBaseView {
  id: root

  required property AutomationTrack automationTrack
  readonly property AutomationClip automationClip: arrangerObject as AutomationClip

  clipContent: AutomationClipContent {
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    automationClip: root.automationClip
  }
}
