// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmArrangement 1.0

ArrangerObjectBaseView {
  id: root

  property MidiNote midiNote: root.arrangerObject

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: 2
  }
}
