// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

ArrangerObjectBaseView {
  id: root

  colorRole: ArrangerObjectBaseView.ColorRole.Content

  readonly property AutomationPoint automationPoint: arrangerObject as AutomationPoint

  selectionCornerRadius: root.height / 2

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: root.height / 2
  }
}
