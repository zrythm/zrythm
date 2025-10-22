// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

ArrangerObjectBaseView {
  id: root

  readonly property AutomationPoint automationPoint: arrangerObject as AutomationPoint

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: root.height / 2
  }
}
