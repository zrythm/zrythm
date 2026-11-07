// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmStyle
import Zrythm

Rectangle {
  id: root

  property int algorithm
  required property AudioEngine audioEngine
  property int channel
  readonly property real currentPx: meterProcessorItem.toFader(meterProcessorItem.currentAmplitude) * levelLength
  // Render the bar left-to-right instead of bottom-to-top.
  property bool horizontal: false
  // K-System-style palette: green below the warning zone, yellow above it.
  // The default palette is the classic orange-to-green gradient. In both
  // palettes the peak marker turns red when the level is over full scale.
  property bool k20Colors: false
  // Current level as a 0-1 fraction of the bar, for segment activation.
  readonly property real levelFraction: levelLength > 0 ? Math.min(Math.max(currentPx / levelLength, 0), 1) : 0
  readonly property real levelLength: horizontal ? width : height
  readonly property alias meterProcessor: meterProcessorItem
  readonly property real peakPx: meterProcessorItem.toFader(meterProcessorItem.peakAmplitude) * levelLength
  required property Port port
  required property PortObservationManager portObservationManager
  // Number of LED segments. 0 or 1 renders one continuous gradient bar.
  property int segmentCount: 0
  // Fraction of the bar where the warning color starts in K-System mode.
  property real warningPos: 0.72

  // Color for a position along the bar (0 = start, 1 = full level).
  function colorAt(pos: real): color {
    if (k20Colors)
      return pos >= warningPos ? "#F7FF11" : "#2BD700";

    const p = Math.min(Math.max(pos, 0), 1);
    if (p < 0.25)
      return lerpColor("#FF9C11", "#F7FF11", p / 0.25);

    return lerpColor("#F7FF11", "#2BD700", (p - 0.25) / 0.75);
  }

  function lerpColor(from: color, to: color, t: real): color {
    return Qt.rgba(from.r + (to.r - from.r) * t, from.g + (to.g - from.g) * t, from.b + (to.b - from.b) * t, 1);
  }

  clip: true
  color: "transparent"
  implicitHeight: horizontal ? 10 : 100
  implicitWidth: horizontal ? 100 : 4
  layer.enabled: true
  layer.smooth: true

  MeterProcessor {
    id: meterProcessorItem

    algorithm: root.algorithm
    channel: root.channel
    port: root.port
    portObservationManager: root.portObservationManager
    sampleRate: root.audioEngine.sampleRate
  }

  // Faint strip behind the segments so off segments stay visible.
  Rectangle {
    anchors.fill: parent
    color: Qt.alpha("#FFFFFF", 0.08)
    radius: 2
    visible: root.segmentCount > 1
  }

  Rectangle {
    id: filledRect

    height: root.horizontal ? root.height : root.currentPx
    visible: root.segmentCount <= 1
    width: root.horizontal ? root.currentPx : root.width
    x: 0
    y: root.horizontal ? 0 : root.height - root.currentPx

    gradient: Gradient {
      orientation: root.horizontal ? Gradient.Horizontal : Gradient.Vertical

      GradientStop {
        color: "#FF9C11"
        position: 0
      }

      GradientStop {
        color: "#F7FF11"
        position: 0.25
      }

      GradientStop {
        color: "#2BD700"
        position: 1
      }
    }
    Behavior on height {
      animation: ZrythmTheme.propertyAnimation
    }
    Behavior on width {
      animation: ZrythmTheme.propertyAnimation
    }
  }

  Rectangle {
    id: peakRect

    readonly property bool isOver: meterProcessorItem.peakAmplitude > 1

    color: isOver ? ZrythmTheme.errorColor : ZrythmTheme.backgroundAppendColor
    height: root.horizontal ? root.height : (isOver ? 2 : 1.5)
    opacity: isOver ? 1 : meterProcessorItem.peakAmplitude
    width: root.horizontal ? (isOver ? 2 : 1.5) : root.width
    x: root.horizontal ? root.peakPx - width : 0
    y: root.horizontal ? 0 : root.height - root.peakPx

    Behavior on x {
      animation: ZrythmTheme.propertyAnimation
    }
    Behavior on y {
      animation: ZrythmTheme.propertyAnimation
    }
  }

  Repeater {
    id: segmentRepeater

    model: root.segmentCount > 1 ? root.segmentCount : 0

    Rectangle {
      id: segment

      readonly property bool active: root.levelFraction >= segment.segmentPos
      required property int index
      readonly property real segmentGap: 2
      readonly property real segmentLength: (root.levelLength - (segmentRepeater.model - 1) * segmentGap) / segmentRepeater.model
      readonly property real segmentPos: (segment.index + 1) / segmentRepeater.model

      color: segment.active ? root.colorAt(segment.segmentPos) : "#484848"
      height: root.horizontal ? root.height : segment.segmentLength
      radius: 1
      width: root.horizontal ? segment.segmentLength : root.width
      x: root.horizontal ? segment.index * (segment.segmentLength + segment.segmentGap) : 0
      y: root.horizontal ? 0 : root.height - (segment.index + 1) * segment.segmentLength - segment.index * segment.segmentGap
    }
  }
}
