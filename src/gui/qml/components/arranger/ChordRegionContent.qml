// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui
import ZrythmStyle

Item {
  id: root

  // FIXME: find a cleaner way to get a MusicalScale instance for calling static methods
  readonly property MusicalScale _scaleHelper: MusicalScale {
  }
  required property int contentHeight
  required property int contentWidth
  readonly property real pxPerTick: root.contentWidth / root.regionTicks
  required property ChordRegion region
  readonly property real regionTicks: root.region.bounds.length.ticks

  function rootNoteToColor(rootNote) {
    return Qt.hsla(rootNote / 12.0, 0.7, 0.5, 0.6);
  }

  ChordRegionSegmenter {
    id: segmenter

    region: root.region
  }

  Repeater {
    model: segmenter.segments

    delegate: Item {
      id: cell

      required property real absEndTicks
      required property real absStartTicks
      required property int chordIndex
      readonly property string chordName: cell.chordObject && cell.chordObject.chordDescriptor ? cell.chordObject.chordDescriptor.displayName : ""
      required property ChordObject chordObject

      // Per-cell LOD: measure the actual full name width and decide what
      // fits. This eliminates the useless "G..." elided state — each cell
      // shows either its full name, root note, or a colored block.
      readonly property real fullNameWidth: nameMetrics.width + 8 // 4px left + right padding
      readonly property int rootNote: cell.chordObject && cell.chordObject.chordDescriptor ? cell.chordObject.chordDescriptor.rootNote : 0
      readonly property string rootNoteName: cell.chordObject && cell.chordObject.chordDescriptor ? root._scaleHelper.noteToString(cell.chordObject.chordDescriptor.rootNote) : ""
      readonly property bool showsFullName: cell.width >= cell.fullNameWidth
      readonly property bool showsRootNote: !cell.showsFullName && cell.width >= 15

      height: root.contentHeight
      width: Math.max(1, (cell.absEndTicks - cell.absStartTicks) * root.pxPerTick)
      x: cell.absStartTicks * root.pxPerTick

      TextMetrics {
        id: nameMetrics

        font: ZrythmTheme.arrangerObjectTextFont
        text: cell.chordName
      }

      // Colored block background — shown when cells are too small for
      // even root note letters. Hue encodes the root note.
      Rectangle {
        anchors.fill: parent
        color: root.rootNoteToColor(cell.rootNote)
        visible: !cell.showsFullName && !cell.showsRootNote
      }

      // Right-edge separator between cells — always drawn so chord
      // boundaries are visible at every zoom level.
      Rectangle {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.top: parent.top
        color: ZrythmTheme.regionContentColor
        opacity: 0.4
        width: 1
      }

      // Full chord name (wide enough cells). No eliding — the TextMetrics
      // guard guarantees it fits.
      Text {
        anchors.fill: parent
        anchors.leftMargin: 4
        color: ZrythmTheme.regionContentColor
        font: ZrythmTheme.arrangerObjectTextFont
        horizontalAlignment: Text.AlignLeft
        text: cell.chordName
        verticalAlignment: Text.AlignVCenter
        visible: cell.showsFullName
      }

      // Root note only (medium cells). Centered, bold for legibility.
      Text {
        anchors.fill: parent
        color: ZrythmTheme.regionContentColor
        font: ZrythmTheme.arrangerObjectBoldTextFont
        horizontalAlignment: Text.AlignHCenter
        text: cell.rootNoteName
        verticalAlignment: Text.AlignVCenter
        visible: cell.showsRootNote
      }
    }
  }
}
