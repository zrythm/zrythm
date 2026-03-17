// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

ArrangerObjectBaseView {
  id: root

  readonly property MidiNote midiNote: arrangerObject as MidiNote
  property bool showVelocityText: false

  edgeHandles.active: false

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: 1

    Text {
      anchors.bottom: parent.top
      anchors.horizontalCenter: parent.horizontalCenter
      color: palette.text
      font.pixelSize: 10
      text: root.midiNote.velocity
      visible: root.showVelocityText
    }
  }
}
