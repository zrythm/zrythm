// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Arranger {
  id: root

  required property var pianoRoll

  function beginObjectCreation(x: real, y: real): var {
    return null;
  }

  editorSettings: pianoRoll.editorSettings
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: ListView {
    id: midiNotesListView

    anchors.fill: parent
    interactive: false
    model: region.midiNotes

    delegate: Item {
      id: midiNoteDelegate

      TextMetrics {
        id: arrangerObjectTextMetrics

        font: Style.arrangerObjectTextFont
        text: "Some text"
      }

      Loader {
        id: midiNotesLoader

      }
    }
  }
}
