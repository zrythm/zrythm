// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes
import ZrythmStyle
import ZrythmDsp
import ZrythmGui
import Qt.labs.synchronizer

Item {
  id: root

  enum CurrentAction {
    None,
    MovingPlayhead,
    MovingLoopStart,
    MovingLoopEnd,
    MovingLoopRange
  }

  readonly property real barLineOpacity: 0.8
  readonly property real beatLineOpacity: 0.6
  readonly property alias contentWidth: scrollView.contentWidth
  property bool ctrlHeld: false
  property int currentAction: Ruler.CurrentAction.None
  readonly property real defaultPxPerTick: 0.03
  readonly property real detailMeasureLabelPxThreshold: 64 // threshold to show/hide labels for more detailed measures
  readonly property real detailMeasurePxThreshold: 32 // threshold to show/hide more detailed measures
  property real dragStartLoopEndTicks: 0
  property real dragStartLoopStartTicks: 0
  property real dragStartX: 0
  required property EditorSettings editorSettings
  readonly property int markerSize: 8
  readonly property int maxBars: 256
  readonly property real maxZoomLevel: 1800
  readonly property real minZoomLevel: 0.04
  readonly property real pxPerBar: pxPerSixteenth * 16
  readonly property real pxPerSixteenth: ticksPerSixteenth * pxPerTick
  readonly property real pxPerTick: defaultPxPerTick * (editorSettings?.horizontalZoomLevel ?? 1)
  property ArrangerObject region: null
  property bool shiftHeld: false
  readonly property bool shouldSnap: !root.shiftHeld && (root.snapGrid.snapToGrid || root.snapGrid.snapToEvents)
  readonly property real sixteenthLineOpacity: 0.4
  required property SnapGrid snapGrid
  required property TempoMap tempoMap
  readonly property real ticksPerSixteenth: tempoMap.getPpq() / 4
  property Track track: null
  required property Transport transport

  function calculateSnappedPosition(currentTicks: real, startTicks: real): real {
    return root.shouldSnap ? root.snapGrid.snapWithStartTicks(currentTicks, startTicks) : currentTicks;
  }

  function getActionAtPosition(x: real, y: real, ctrlPressed: bool): int {
    if (!root.transport.loopEnabled)
      return Ruler.CurrentAction.MovingPlayhead;

    const loopStartX = loopRange.startX;
    const loopEndX = loopRange.endX;
    const markerWidth = loopRange.loopMarkerWidth;

    // Check if over loop start marker
    if (x >= loopStartX && x <= loopStartX + markerWidth && y <= loopRange.loopMarkerHeight)
      return Ruler.CurrentAction.MovingLoopStart;

    // Check if over loop end marker
    if (x >= loopEndX - markerWidth && x <= loopEndX && y <= loopRange.loopMarkerHeight)
      return Ruler.CurrentAction.MovingLoopEnd;

    // Check if over loop range (only when Ctrl is held)
    if (x >= loopStartX && x <= loopEndX && ctrlPressed)
      return Ruler.CurrentAction.MovingLoopRange;

    return Ruler.CurrentAction.MovingPlayhead;
  }

  function ticksFromX(x: real): real {
    return x / (root.defaultPxPerTick * root.editorSettings.horizontalZoomLevel);
  }

  clip: true
  implicitHeight: 24
  implicitWidth: 64

  ScrollView {
    id: scrollView

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
    anchors.fill: parent
    clip: true
    contentHeight: scrollView.height
    contentWidth: root.maxBars * root.pxPerBar

    Synchronizer {
      sourceObject: root.editorSettings
      sourceProperty: "x"
      targetObject: scrollView.contentItem
      targetProperty: "contentX"
    }

    // Grid lines and labels
    RulerGridCanvas {
      height: scrollView.height
      width: scrollView.width
      x: root.editorSettings?.x ?? 0

      barLabelFont: Style.smallTextFont
      barLineOpacity: root.barLineOpacity
      beatLabelFont: Style.xSmallTextFont
      beatLineOpacity: root.beatLineOpacity
      detailMeasureLabelPxThreshold: root.detailMeasureLabelPxThreshold
      detailMeasurePxThreshold: root.detailMeasurePxThreshold
      pxPerTick: root.pxPerTick
      scrollX: root.editorSettings?.x ?? 0
      scrollXPlusWidth: (root.editorSettings?.x ?? 0) + scrollView.width
      sixteenthLabelFont: Style.xxSmallTextFont
      sixteenthLineOpacity: root.sixteenthLineOpacity
      tempoMap: root.tempoMap
      textColor: root.palette.text
    }

    Item {
      id: markers

      height: parent.height

      PlayheadTriangle {
        id: playheadShape

        height: 8
        width: 12
        x: root.transport.playhead.ticks * root.pxPerTick - width / 2
        y: root.height - height
      }

      Loader {
        id: regionMarkersLoader

        active: root.region !== null
        anchors.fill: parent
        enabled: active
        visible: active

        sourceComponent: Item {
          x: root.region.position.ticks * root.pxPerTick

          Rectangle {
            id: regionBackground

            anchors.bottom: parent.bottom
            color: root.region.color.useColor ? root.region.color.color : root.track.color
            height: 8
            opacity: 0.6
            width: root.region.bounds.length.ticks * root.pxPerTick
          }

          Rectangle {
            id: regionClipStartRect

            anchors.bottom: parent.bottom
            color: "red"
            height: 12
            width: 2
            x: root.region.loopRange.clipStartPosition.ticks * root.pxPerTick - width / 2
          }

          Rectangle {
            id: regionLoopStartRect

            anchors.bottom: parent.bottom
            color: "green"
            height: 12
            width: 2
            x: root.region.loopRange.loopStartPosition.ticks * root.pxPerTick - width / 2
          }

          Rectangle {
            id: regionLoopEndRect

            anchors.bottom: parent.bottom
            color: "green"
            height: 12
            width: 2
            x: root.region.loopRange.loopEndPosition.ticks * root.pxPerTick - width / 2
          }
        }
      }

      Item {
        id: loopRange

        readonly property real endX: root.transport.loopEndPosition.ticks * root.defaultPxPerTick * root.editorSettings.horizontalZoomLevel
        readonly property real loopMarkerHeight: 8
        readonly property real loopMarkerWidth: 10
        readonly property real startX: root.transport.loopStartPosition.ticks * root.defaultPxPerTick * root.editorSettings.horizontalZoomLevel

        Shape {
          id: loopStartShape

          antialiasing: true
          height: loopRange.loopMarkerHeight
          layer.enabled: true
          layer.samples: 4
          visible: root.transport.loopEnabled
          width: loopRange.loopMarkerWidth
          x: loopRange.startX
          z: 10

          ShapePath {
            fillColor: root.palette.accent
            strokeColor: root.palette.accent

            PathLine {
              x: 0
              y: 0
            }

            PathLine {
              x: loopStartShape.width
              y: 0
            }

            PathLine {
              x: 0
              y: loopStartShape.height
            }
          }
        }

        Shape {
          id: loopEndShape

          antialiasing: true
          height: loopRange.loopMarkerHeight
          layer.enabled: true
          layer.samples: 4
          visible: root.transport.loopEnabled
          width: loopRange.loopMarkerWidth
          x: loopRange.endX - loopRange.loopMarkerWidth
          z: 10

          ShapePath {
            fillColor: root.palette.accent
            strokeColor: root.palette.accent

            PathLine {
              x: 0
              y: 0
            }

            PathLine {
              x: loopEndShape.width
              y: 0
            }

            PathLine {
              x: loopEndShape.width
              y: loopEndShape.height
            }
          }
        }

        Rectangle {
          id: loopRangeRect

          color: Qt.alpha(root.palette.accent, 0.1)
          height: root.height
          visible: root.transport.loopEnabled
          width: loopRange.endX - loopRange.startX
          x: loopRange.startX
        }
      }
    }

    MouseArea {
      id: rulerMouseArea

      acceptedButtons: Qt.LeftButton | Qt.RightButton
      anchors.fill: parent
      hoverEnabled: true
      preventStealing: true

      onPositionChanged: mouse => {
        // Track modifiers
        root.ctrlHeld = mouse.modifiers & Qt.ControlModifier;
        root.shiftHeld = mouse.modifiers & Qt.ShiftModifier;

        // Update cursor based on hover position
        if (!pressed) {
          const hoverAction = root.getActionAtPosition(mouse.x, mouse.y, root.ctrlHeld);
          if (hoverAction === Ruler.CurrentAction.MovingLoopStart || hoverAction === Ruler.CurrentAction.MovingLoopEnd) {
            cursorShape = Qt.SizeHorCursor;
          } else if (hoverAction === Ruler.CurrentAction.MovingLoopRange) {
            cursorShape = Qt.SizeAllCursor;
          } else {
            cursorShape = Qt.ArrowCursor;
          }
          return;
        }

        // Handle drag actions
        const currentTicks = root.ticksFromX(mouse.x);
        const tickDelta = root.ticksFromX(mouse.x - root.dragStartX);

        switch (root.currentAction) {
        case Ruler.CurrentAction.MovingPlayhead:
          root.transport.playhead.ticks = root.calculateSnappedPosition(currentTicks, root.ticksFromX(root.dragStartX));
          break;
        case Ruler.CurrentAction.MovingLoopStart:
          const newStartTicks = root.dragStartLoopStartTicks + tickDelta;
          const snappedStartTicks = root.calculateSnappedPosition(newStartTicks, root.dragStartLoopStartTicks);
          // Clamp: start must be >= 0 and < end position
          root.transport.loopStartPosition.ticks = Math.max(0, Math.min(snappedStartTicks, root.transport.loopEndPosition.ticks - 1));
          break;
        case Ruler.CurrentAction.MovingLoopEnd:
          const newEndTicks = root.dragStartLoopEndTicks + tickDelta;
          const snappedEndTicks = root.calculateSnappedPosition(newEndTicks, root.dragStartLoopEndTicks);
          // Clamp: end must be > start position
          root.transport.loopEndPosition.ticks = Math.max(snappedEndTicks, root.transport.loopStartPosition.ticks + 1);
          break;
        case Ruler.CurrentAction.MovingLoopRange:
          const loopLength = root.dragStartLoopEndTicks - root.dragStartLoopStartTicks;
          const newRangeStartTicks = root.dragStartLoopStartTicks + tickDelta;
          const snappedRangeStartTicks = root.calculateSnappedPosition(newRangeStartTicks, root.dragStartLoopStartTicks);
          const clampedStartTicks = Math.max(0, snappedRangeStartTicks);
          root.transport.loopStartPosition.ticks = clampedStartTicks;
          root.transport.loopEndPosition.ticks = clampedStartTicks + loopLength;
          break;
        }
      }
      onPressed: mouse => {
        // Track modifiers
        root.ctrlHeld = mouse.modifiers & Qt.ControlModifier;
        root.shiftHeld = mouse.modifiers & Qt.ShiftModifier;

        if (mouse.button === Qt.LeftButton) {
          root.dragStartX = mouse.x;
          root.dragStartLoopStartTicks = root.transport.loopStartPosition.ticks;
          root.dragStartLoopEndTicks = root.transport.loopEndPosition.ticks;
          root.currentAction = root.getActionAtPosition(mouse.x, mouse.y, root.ctrlHeld);

          // For playhead, also set initial position on press
          if (root.currentAction === Ruler.CurrentAction.MovingPlayhead) {
            root.transport.movePlayhead(root.calculateSnappedPosition(root.ticksFromX(mouse.x), root.ticksFromX(mouse.x)), true);
          }
        }
      }
      onReleased: {
        root.currentAction = Ruler.CurrentAction.None;
      }
      onWheel: wheel => {
        if (wheel.modifiers & Qt.ControlModifier) {
          const multiplier = wheel.angleDelta.y > 0 ? 1.3 : 1 / 1.3;
          root.editorSettings.horizontalZoomLevel = Math.min(Math.max(root.editorSettings.horizontalZoomLevel * multiplier, root.minZoomLevel), root.maxZoomLevel);
        }
      }
    }
  }
}
