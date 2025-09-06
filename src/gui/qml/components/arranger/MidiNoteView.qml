// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

ArrangerObjectBaseView {
  id: root

  property MidiNote midiNote: root.arrangerObject as MidiNote

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: 2
  }
}
