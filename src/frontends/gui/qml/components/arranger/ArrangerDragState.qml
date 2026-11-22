// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick

// Shareable drag state passed between arrangers that must react to each
// other's drags (e.g. MidiArranger + VelocityArranger). Owns the DragMode
// enum so the mode concept lives with the state.
QtObject {
  enum DragMode {
    None,
    Move,
    ResizeFromStart,
    ResizeFromEnd,
    Velocity
  }

  property real dragDeltaPx: 0
  property real dragDeltaY: 0
  property int dragMode: ArrangerDragState.DragMode.None

  // True when the current resize is a loop-point resize (Ctrl+drag on the
  // edge handles). When true, content stays in place and loop repetitions
  // fill the extended area, instead of clipping/shifting the content.
  property bool isLoopResize: false

  // Clear drag mode and accumulated deltas (e.g. on drag release).
  function reset() {
    dragMode = ArrangerDragState.DragMode.None;
    dragDeltaPx = 0;
    dragDeltaY = 0;
    isLoopResize = false;
  }
}
