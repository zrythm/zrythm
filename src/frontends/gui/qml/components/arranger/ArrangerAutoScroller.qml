// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

// Repeatedly scrolls an EditorSettings-linked viewport while the pointer
// rests inside an edge zone during an arranger drag. Emits scrolled() with
// the deltas actually applied (after clamping to the scrollable range) so
// the caller can compensate active drag coordinates for the moved
// viewport.
QtObject {
  id: root

  enum Axes {
    None,
    Horizontal,
    Vertical,
    Both
  }

  // Scroll position holder of the viewport to scroll
  property EditorSettings editorSettings: null

  // Whether a drag that may auto-scroll is in progress
  property bool active: false

  // Pointer position relative to the viewport. Positions slightly outside
  // the viewport count: a drag that holds the pointer beyond an edge keeps
  // scrolling.
  property point pointer: Qt.point(0, 0)
  property bool pointerValid: false

  // Axes the current drag action may scroll
  property int axes: ArrangerAutoScroller.Axes.Both

  // Viewport size and scrollable extent (content size minus viewport size)
  property real viewportWidth: 0
  property real viewportHeight: 0
  property real maxScrollX: 0
  property real maxScrollY: 0

  // Depth of the edge zones and scroll speed at their outermost pixel, in
  // content pixels per tick. Small values keep the zones from swallowing
  // usable edit space on small viewports.
  readonly property real edgeMargin: 16
  readonly property real maxSpeed: 12

  signal scrolled(real deltaX, real deltaY)

  property Timer timer: Timer {
    interval: 16
    repeat: true
    running: root.active && root.pointerValid && root.editorSettings !== null

    onTriggered: root.tick()
  }

  // Maps an arranger drag action to the axes it may auto-scroll: drags
  // that reposition, create-move, select or ramp objects scroll both axes,
  // purely horizontal drags (resizes, stretches, auditions) scroll
  // horizontally, and vertical-value drags scroll vertically.
  function axesForArrangerAction(action: int): int {
    switch (action) {
    case Arranger.Moving:
    case Arranger.MovingCopy:
    case Arranger.MovingLink:
    case Arranger.CreatingMoving:
    case Arranger.CreatingResizingMovingR:
    case Arranger.Selecting:
    case Arranger.DeleteSelecting:
    case Arranger.Ramping:
      return ArrangerAutoScroller.Axes.Both;
    case Arranger.CreatingResizingR:
    case Arranger.ResizingL:
    case Arranger.ResizingLLoop:
    case Arranger.ResizingLFade:
    case Arranger.ResizingR:
    case Arranger.ResizingRLoop:
    case Arranger.ResizingRFade:
    case Arranger.StretchingL:
    case Arranger.StretchingR:
    case Arranger.Auditioning:
      return ArrangerAutoScroller.Axes.Horizontal;
    case Arranger.ResizingUp:
    case Arranger.ResizingUpFadeIn:
    case Arranger.ResizingUpFadeOut:
      return ArrangerAutoScroller.Axes.Vertical;
    default:
      return ArrangerAutoScroller.Axes.None;
    }
  }

  // Scrolls one step according to the pointer's position within the edge
  // zones. The timer drives repeated steps while a drag holds the pointer
  // inside a zone.
  function tick() {
    if (!pointerValid || editorSettings === null)
      return;
    let fractionX = 0;
    if ((axes & ArrangerAutoScroller.Axes.Horizontal) !== 0 && viewportWidth > 0) {
      const margin = Math.min(edgeMargin, viewportWidth / 4);
      if (pointer.x > viewportWidth - margin)
        fractionX = (pointer.x - (viewportWidth - margin)) / margin;
      else if (pointer.x < margin)
        fractionX = (pointer.x - margin) / margin;
    }
    let fractionY = 0;
    if ((axes & ArrangerAutoScroller.Axes.Vertical) !== 0 && viewportHeight > 0) {
      const margin = Math.min(edgeMargin, viewportHeight / 4);
      if (pointer.y > viewportHeight - margin)
        fractionY = (pointer.y - (viewportHeight - margin)) / margin;
      else if (pointer.y < margin)
        fractionY = (pointer.y - margin) / margin;
    }
    const oldX = editorSettings.x;
    const oldY = editorSettings.y;
    const stepX = boundedStep(oldX, Math.max(-1, Math.min(1, fractionX)) * maxSpeed, maxScrollX);
    const stepY = boundedStep(oldY, Math.max(-1, Math.min(1, fractionY)) * maxSpeed, maxScrollY);
    if (stepX === 0 && stepY === 0)
      return;
    if (stepX !== 0)
      editorSettings.x = oldX + stepX;
    if (stepY !== 0)
      editorSettings.y = oldY + stepY;
    scrolled(stepX, stepY);
  }

  // Clamps one scroll step so the position stays inside [0, maxScroll]. A
  // position already outside the range (the content shrank below the
  // viewport) is left untouched.
  function boundedStep(current: real, step: real, maxScroll: real): real {
    const maxValue = Math.max(maxScroll, 0);
    if (current < 0 || current > maxValue)
      return 0;
    if (step > 0)
      return Math.min(step, maxValue - current);
    return Math.max(step, -current);
  }
}
