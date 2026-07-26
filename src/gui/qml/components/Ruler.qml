// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
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
    MovingLoopRange,
    MovingClipStart,
    MovingClipLoopStart,
    MovingClipLoopEnd
  }

  readonly property real barLineOpacity: 0.8
  readonly property real beatLineOpacity: 0.6
  property Clip clipObject: null
  /// Session-owned operator for clip loop-point edits, injected by the editor
  /// pane. Null where clip markers are unused (e.g. the timeline ruler).
  property ClipOperator clipOperator: null
  // Absolute (content-item) pixel x of the clip's loop markers, kept in sync
  // with clip positions / warp / zoom for both rendering and hit-testing.
  property real clipStartMarkerX: 0
  readonly property alias contentWidth: scrollView.contentWidth
  property bool ctrlHeld: false
  property int currentAction: Ruler.CurrentAction.None
  readonly property real defaultPxPerTick: 0.034
  readonly property real detailMeasureLabelPxThreshold: 64 // threshold to show/hide labels for more detailed measures
  readonly property real detailMeasurePxThreshold: 32 // threshold to show/hide more detailed measures
  property real dragStartClipLoopEndTicks: 0
  property real dragStartClipLoopStartTicks: 0
  property real dragStartClipStartTicks: 0
  property real dragStartLoopEndTicks: 0
  property real dragStartLoopStartTicks: 0
  property real dragStartX: 0
  required property EditorSettings editorSettings
  property real loopEndMarkerX: 0
  property real loopStartMarkerX: 0
  readonly property int markerSize: 8
  readonly property int maxBars: 256
  readonly property real maxZoomLevel: 1800
  readonly property real minZoomLevel: 0.04
  property int playbackCacheCompleteCount: 0
  property int playbackCachePendingCount: 0
  readonly property real pxPerBar: pxPerSixteenth * 16
  readonly property real pxPerSixteenth: ticksPerSixteenth * pxPerTick
  readonly property real pxPerTick: defaultPxPerTick * (editorSettings?.horizontalZoomLevel ?? 1)
  property bool shiftHeld: false
  readonly property bool shouldSnap: !root.shiftHeld && (root.snapGrid.snapToGrid || root.snapGrid.snapToEvents)
  readonly property real sixteenthLineOpacity: 0.4
  required property SnapGrid snapGrid
  required property TempoMap tempoMap
  readonly property real ticksPerSixteenth: tempoMap.getPpq() / 4
  property Track track: null
  required property Transport transport

  function calculateSnappedTimelinePosition(currentTicks: real, startTicks: real): real {
    // Clamp: timeline positions must be at or after the timeline origin
    const clampedTicks = Math.max(0, currentTicks);
    return root.shouldSnap ? root.snapGrid.snapWithStartTicks(clampedTicks, startTicks) : clampedTicks;
  }

  /// Converts an absolute content-item x (mouse.x) to clip content ticks.
  function contentTicksAt(x: real): real {
    const absTimelineTicks = root.ticksFromX(x);
    const relTimelineTicks = absTimelineTicks - root.clipObject.position.ticks;
    const warp = root.clipObject.contentWarp;
    return warp ? warp.timelineTicksRelativeToContent(relTimelineTicks) : relTimelineTicks;
  }

  function getActionAtPosition(x: real, y: real, ctrlPressed: bool): int {
    // Clip loop markers take priority in the clip editor (they hang from the
    // top edge, so only grab them near the top of the ruler). Brackets take
    // precedence over the clip-start pill where they overlap.
    if (root.clipObject !== null && y <= 18) {
      if (Math.abs(x - root.loopStartMarkerX) <= 7)
        return Ruler.CurrentAction.MovingClipLoopStart;
      if (Math.abs(x - root.loopEndMarkerX) <= 7)
        return Ruler.CurrentAction.MovingClipLoopEnd;
      if (x >= root.clipStartMarkerX - 5 && x <= root.clipStartMarkerX + 44)
        return Ruler.CurrentAction.MovingClipStart;
    }

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
    return x / root.pxPerTick;
  }

  /// Recomputes the absolute pixel positions of the clip's loop markers.
  function updateClipMarkerGeometry() {
    if (!root.clipObject)
      return;
    const clipPosPx = root.clipObject.position.ticks * root.pxPerTick;
    const warp = root.clipObject.contentWarp;
    const cs = root.clipObject.clipStartPosition.ticks;
    const ls = root.clipObject.loopStartPosition.ticks;
    const le = root.clipObject.loopEndPosition.ticks;
    const csRel = warp ? warp.contentToTimelineTicksRelative(cs) : cs;
    const lsRel = warp ? warp.contentToTimelineTicksRelative(ls) : ls;
    const leRel = warp ? warp.contentToTimelineTicksRelative(le) : le;
    root.clipStartMarkerX = clipPosPx + csRel * root.pxPerTick;
    root.loopStartMarkerX = clipPosPx + lsRel * root.pxPerTick;
    root.loopEndMarkerX = clipPosPx + leRel * root.pxPerTick;
  }

  clip: true
  implicitHeight: 24
  implicitWidth: 64

  Component.onCompleted: {
    root.updateClipMarkerGeometry();
    if (root.clipOperator)
      root.clipOperator.clip = root.clipObject;
  }
  onClipObjectChanged: {
    root.updateClipMarkerGeometry();
    if (root.clipOperator)
      root.clipOperator.clip = root.clipObject;
  }
  onPxPerTickChanged: root.updateClipMarkerGeometry()

  // Keep the clip marker pixel positions in sync for rendering + hit-testing.
  Connections {
    function onPositionChanged() {
      root.updateClipMarkerGeometry();
    }

    target: root.clipObject?.position ?? null
  }

  Connections {
    function onTimelineLengthTicksChanged() {
      root.updateClipMarkerGeometry();
    }

    target: root.clipObject
  }

  Connections {
    function onPositionChanged() {
      root.updateClipMarkerGeometry();
    }

    target: root.clipObject?.clipStartPosition ?? null
  }

  Connections {
    function onPositionChanged() {
      root.updateClipMarkerGeometry();
    }

    target: root.clipObject?.loopStartPosition ?? null
  }

  Connections {
    function onPositionChanged() {
      root.updateClipMarkerGeometry();
    }

    target: root.clipObject?.loopEndPosition ?? null
  }

  Connections {
    function onMapChanged() {
      root.updateClipMarkerGeometry();
    }

    target: root.clipObject?.contentWarp ?? null
  }

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
      barLabelFont: ZrythmTheme.smallTextFont
      barLineOpacity: root.barLineOpacity
      beatLabelFont: ZrythmTheme.xSmallTextFont
      beatLineOpacity: root.beatLineOpacity
      detailMeasureLabelPxThreshold: root.detailMeasureLabelPxThreshold
      detailMeasurePxThreshold: root.detailMeasurePxThreshold
      height: scrollView.height
      pxPerTick: root.pxPerTick
      scrollX: root.editorSettings?.x ?? 0
      scrollXPlusWidth: (root.editorSettings?.x ?? 0) + scrollView.width
      sixteenthLabelFont: ZrythmTheme.xxSmallTextFont
      sixteenthLineOpacity: root.sixteenthLineOpacity
      tempoMap: root.tempoMap
      textColor: root.palette.text
      width: scrollView.width
      x: root.editorSettings?.x ?? 0
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

        active: root.clipObject !== null
        anchors.fill: parent
        enabled: active
        visible: active

        sourceComponent: Item {
          // Tinted band spanning the loop region [loop-start, loop-end].
          Rectangle {
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            color: Qt.rgba(0.298, 0.686, 0.314, 0.12)
            width: Math.abs(root.loopEndMarkerX - root.loopStartMarkerX)
            x: Math.min(root.loopStartMarkerX, root.loopEndMarkerX)
          }

          // Clip color strip marking the clip's timeline extent.
          Rectangle {
            id: regionBackground

            anchors.bottom: parent.bottom
            color: root.clipObject.color.useColor ? root.clipObject.color.color : (root.track ? root.track.color : "gray")
            height: 8
            opacity: 0.6
            width: root.clipObject.timelineLengthTicks * root.pxPerTick
            x: root.clipObject.position.ticks * root.pxPerTick
          }

          // Clip-start: a labeled red tab hanging from the top edge, starting
          // at the clip-start position (left edge aligned to the marker).
          Item {
            height: 16
            width: 42
            x: root.clipStartMarkerX
            y: 0

            Rectangle {
              anchors.fill: parent
              border.color: "#e05555"
              border.width: 1
              color: Qt.rgba(0.227, 0.133, 0.133, 1)
              radius: 3
            }

            Text {
              anchors.centerIn: parent
              color: "#e3a0a0"
              font: ZrythmTheme.xSmallTextFont
              text: qsTr("Start")
            }

            // Guide line down the ruler, at the clip-start marker.
            Rectangle {
              color: "#e05555"
              height: root.height - parent.height
              opacity: 0.5
              width: 2
              x: -1
              y: parent.height
            }
          }

          // Loop-start bracket: vertical line + top cap opening right.
          Item {
            height: root.height
            width: 1
            x: root.loopStartMarkerX
            y: 0

            Rectangle {
              color: "#4caf50"
              height: parent.height
              width: 2
              x: -1
              y: 0
            }

            Rectangle {
              color: "#4caf50"
              height: 3
              width: 12
              x: -1
              y: 0
            }
          }

          // Loop-end bracket: vertical line + top cap opening left.
          Item {
            height: root.height
            width: 1
            x: root.loopEndMarkerX
            y: 0

            Rectangle {
              color: "#4caf50"
              height: parent.height
              width: 2
              x: -1
              y: 0
            }

            Rectangle {
              color: "#4caf50"
              height: 3
              width: 12
              x: -11
              y: 0
            }
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

      // A stolen/canceled gesture (touch recognizer, popup hide) reverts the
      // live preview instead of committing it, and clears any global cursor
      // override so the cursor does not get stuck.
      onCanceled: {
        if (root.currentAction === Ruler.CurrentAction.MovingClipStart || root.currentAction === Ruler.CurrentAction.MovingClipLoopStart || root.currentAction === Ruler.CurrentAction.MovingClipLoopEnd) {
          root.clipOperator.abortClipLoopPointsDrag();
        }
        CursorManager.unsetCursor();
        root.currentAction = Ruler.CurrentAction.None;
      }
      onExited: {
        // Clear any global cursor override (loop-resize cursors) when the mouse
        // leaves the ruler — the per-item cursorShape resets on its own.
        CursorManager.unsetCursor();
      }
      onPositionChanged: mouse => {
        // Track modifiers
        root.ctrlHeld = mouse.modifiers & Qt.ControlModifier;
        root.shiftHeld = mouse.modifiers & Qt.ShiftModifier;

        // Update cursor based on hover position
        if (!pressed) {
          const hoverAction = root.getActionAtPosition(mouse.x, mouse.y, root.ctrlHeld);
          if (hoverAction === Ruler.CurrentAction.MovingClipLoopStart) {
            CursorManager.setResizeLoopStartCursor();
          } else if (hoverAction === Ruler.CurrentAction.MovingClipLoopEnd) {
            CursorManager.setResizeLoopEndCursor();
          } else {
            // Remove any global override so the per-item cursorShape applies.
            CursorManager.unsetCursor();
            if (hoverAction === Ruler.CurrentAction.MovingClipStart) {
              cursorShape = Qt.OpenHandCursor;
            } else if (hoverAction === Ruler.CurrentAction.MovingLoopStart || hoverAction === Ruler.CurrentAction.MovingLoopEnd) {
              cursorShape = Qt.SizeHorCursor;
            } else if (hoverAction === Ruler.CurrentAction.MovingLoopRange) {
              cursorShape = Qt.SizeAllCursor;
            } else {
              cursorShape = Qt.ArrowCursor;
            }
          }
          return;
        }

        // Handle drag actions
        const currentTicks = root.ticksFromX(mouse.x);
        const tickDelta = root.ticksFromX(mouse.x - root.dragStartX);

        switch (root.currentAction) {
        case Ruler.CurrentAction.MovingPlayhead:
          root.transport.playhead.ticks = root.calculateSnappedTimelinePosition(currentTicks, root.ticksFromX(root.dragStartX));
          break;
        case Ruler.CurrentAction.MovingLoopStart:
          const newStartTicks = root.dragStartLoopStartTicks + tickDelta;
          const snappedStartTicks = root.calculateSnappedTimelinePosition(newStartTicks, root.dragStartLoopStartTicks);
          // Clamp: start must be >= 0 and < end position
          root.transport.loopStartPosition.ticks = Math.max(0, Math.min(snappedStartTicks, root.transport.loopEndPosition.ticks - 1));
          break;
        case Ruler.CurrentAction.MovingLoopEnd:
          const newEndTicks = root.dragStartLoopEndTicks + tickDelta;
          const snappedEndTicks = root.calculateSnappedTimelinePosition(newEndTicks, root.dragStartLoopEndTicks);
          // Clamp: end must be > start position
          root.transport.loopEndPosition.ticks = Math.max(snappedEndTicks, root.transport.loopStartPosition.ticks + 1);
          break;
        case Ruler.CurrentAction.MovingLoopRange:
          const loopLength = root.dragStartLoopEndTicks - root.dragStartLoopStartTicks;
          const newRangeStartTicks = root.dragStartLoopStartTicks + tickDelta;
          const snappedRangeStartTicks = root.calculateSnappedTimelinePosition(newRangeStartTicks, root.dragStartLoopStartTicks);
          const clampedStartTicks = Math.max(0, snappedRangeStartTicks);
          root.transport.loopStartPosition.ticks = clampedStartTicks;
          root.transport.loopEndPosition.ticks = clampedStartTicks + loopLength;
          break;
        case Ruler.CurrentAction.MovingClipStart:
          {
            const ct = root.contentTicksAt(mouse.x);
            const snapped = root.calculateSnappedTimelinePosition(ct, root.dragStartClipStartTicks);
            // Clamp: clip-start must stay in [0, loop-end - 1] so the echoed
            // (non-dragged) loop positions are not forced to move.
            const clamped = Math.max(0, Math.min(snapped, root.dragStartClipLoopEndTicks - 1));
            // Echo the pre-drag values for the non-dragged axes. Using the live
            // positions would feed already-clamped values back through the
            // clip's cross-position constraints and make the markers drift.
            root.clipOperator.updateClipLoopPointsDrag(clamped, root.dragStartClipLoopStartTicks, root.dragStartClipLoopEndTicks);
            break;
          }
        case Ruler.CurrentAction.MovingClipLoopStart:
          {
            const ct = root.contentTicksAt(mouse.x);
            const snapped = root.calculateSnappedTimelinePosition(ct, root.dragStartClipLoopStartTicks);
            // Clamp: loop-start must stay in [0, loop-end - 1].
            const clamped = Math.max(0, Math.min(snapped, root.dragStartClipLoopEndTicks - 1));
            root.clipOperator.updateClipLoopPointsDrag(root.dragStartClipStartTicks, clamped, root.dragStartClipLoopEndTicks);
            break;
          }
        case Ruler.CurrentAction.MovingClipLoopEnd:
          {
            const ct = root.contentTicksAt(mouse.x);
            const snapped = root.calculateSnappedTimelinePosition(ct, root.dragStartClipLoopEndTicks);
            // Clamp: loop-end must stay > max(clip-start, loop-start).
            const minEnd = Math.max(root.dragStartClipStartTicks, root.dragStartClipLoopStartTicks) + 1;
            const clamped = Math.max(snapped, minEnd);
            root.clipOperator.updateClipLoopPointsDrag(root.dragStartClipStartTicks, root.dragStartClipLoopStartTicks, clamped);
            break;
          }
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
            root.transport.movePlayhead(root.calculateSnappedTimelinePosition(root.ticksFromX(mouse.x), root.ticksFromX(mouse.x)), true);
          }

          // Clip loop marker drag: capture content-tick starts and begin gesture.
          if (root.currentAction === Ruler.CurrentAction.MovingClipStart || root.currentAction === Ruler.CurrentAction.MovingClipLoopStart || root.currentAction === Ruler.CurrentAction.MovingClipLoopEnd) {
            root.dragStartClipStartTicks = root.clipObject.clipStartPosition.ticks;
            root.dragStartClipLoopStartTicks = root.clipObject.loopStartPosition.ticks;
            root.dragStartClipLoopEndTicks = root.clipObject.loopEndPosition.ticks;
            root.clipOperator.beginClipLoopPointsDrag();
            // Closed-hand grab cursor while dragging the clip-start pill.
            if (root.currentAction === Ruler.CurrentAction.MovingClipStart) {
              cursorShape = Qt.ClosedHandCursor;
            }
          }
        }
      }
      onReleased: {
        if (root.currentAction === Ruler.CurrentAction.MovingClipStart || root.currentAction === Ruler.CurrentAction.MovingClipLoopStart || root.currentAction === Ruler.CurrentAction.MovingClipLoopEnd) {
          root.clipOperator.endClipLoopPointsDrag();
        }
        root.currentAction = Ruler.CurrentAction.None;
      }
      onWheel: wheel => {
        if (wheel.modifiers & Qt.ControlModifier) {
          const multiplier = wheel.angleDelta.y > 0 ? 1.3 : 1 / 1.3;
          const newZoomLevel = Math.min(Math.max(root.editorSettings.horizontalZoomLevel * multiplier, root.minZoomLevel), root.maxZoomLevel);

          // wheel.x is in content coordinates (Flickable space)
          const tickUnderCursor = wheel.x / root.pxPerTick;
          const cursorViewportOffset = wheel.x - root.editorSettings.x;
          const newPxPerTick = root.defaultPxPerTick * newZoomLevel;
          root.editorSettings.horizontalZoomLevel = newZoomLevel;
          root.editorSettings.x = tickUnderCursor * newPxPerTick - cursorViewportOffset;
        }
      }
    }
  }

  // Cache activity summary bar (debug only, overlay at bottom of ruler)
  Loader {
    active: GlobalState.application.appSettings.showCacheActivity && (root.playbackCachePendingCount > 0 || root.playbackCacheCompleteCount > 0)
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    height: 3
    visible: active
    z: 100

    sourceComponent: Rectangle {
      color: root.playbackCachePendingCount > 0 ? Qt.rgba(1, 0.647, 0, 0.6) : Qt.rgba(0.392, 0.784, 0.392, 0.6)
    }
  }
}
