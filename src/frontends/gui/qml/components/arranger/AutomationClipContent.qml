// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm
import ZrythmGui

Item {
  id: root

  // Ink for the curve, derived from the clip/track color (DESIGN.md "Clips")
  property color inkColor
  required property AutomationClip automationClip
  required property int contentHeight
  required property int contentWidth
  property bool loopPreview: false
  property real referenceWidth: 0
  property real referenceX: 0

  AutomationClipCanvas {
    automationClip: root.automationClip
    curveColor: root.inkColor
    height: root.contentHeight
    loopPreview: root.loopPreview
    referenceWidth: root.referenceWidth
    referenceX: root.referenceX
    width: root.contentWidth
  }
}
