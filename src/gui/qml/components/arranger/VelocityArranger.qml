// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

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

  content: Repeater {
    id: midiNotesRepeater

    anchors.fill: parent
    model: root.clipEditor.region.midiNotes

    delegate: ArrangerObjectLoader {
      id: velocityLoader

      arrangerSelectionModel: root.arrangerSelectionModel
      model: midiNotesRepeater.model
      pxPerTick: root.ruler.pxPerTick
      scrollViewWidth: root.scrollViewWidth
      scrollX: root.scrollX
      unifiedObjectsModel: root.unifiedObjectsModel
      height: 20

      sourceComponent: Component {
        Rectangle {
          id: velocityComponent
          property MidiNote midiNote: velocityLoader.arrangerObject as MidiNote

          anchors {
            bottom: velocityLoader.bottom
            left: velocityLoader.left
          }
          color: root.clipEditor.region.useColor ? root.clipEditor.region.color : root.clipEditor.track.color
          width: 2
          height: parent.height

          Text {
            anchors.centerIn: parent
            text: velocityComponent.midiNote.velocity
            color: palette.highlightedText
          }
        }
      }
    }
  }
}
