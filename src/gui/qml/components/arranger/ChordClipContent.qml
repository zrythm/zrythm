// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm
import ZrythmGui
import ZrythmStyle

Item {
  id: root

  // Chord-name text: the clip body's polarity ink, guaranteed 4.5:1
  // against the body (DESIGN.md "Clips" — text follows the polarity
  // rule; graphics-only ink cannot, on warm bodies)
  property color textColor
  required property ChordClip chordClip
  required property int contentHeight
  required property int contentWidth
  property bool loopPreview: false
  property real referenceWidth: 0
  property real referenceX: 0

  ChordClipCanvas {
    chordClip: root.chordClip
    font: ZrythmTheme.arrangerObjectTextFont
    height: root.contentHeight
    loopPreview: root.loopPreview
    referenceWidth: root.referenceWidth
    referenceX: root.referenceX
    textColor: root.textColor
    width: root.contentWidth
  }
}
