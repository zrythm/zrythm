// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui

Loader {
  id: root

  readonly property bool _dragActive: selectionTracker.isSelected && dragMode !== ArrangerDragState.DragMode.None && dragMode !== ArrangerDragState.DragMode.Velocity
  required property ArrangerObject arrangerObject
  required property ItemSelectionModel arrangerSelectionModel
  // When true, the delegate is centered on its position instead of having its
  // top-left corner there (used by point-like objects such as automation points).
  property bool centeredOnPosition: false
  property real dragDeltaPx: 0
  property real dragDeltaY: 0
  property int dragMode: ArrangerDragState.DragMode.None
  required property int index
  property bool isLoopResize: false
  required property var model
  readonly property real objectEndX: x + width
  readonly property real objectX: {
    const base = posTracker.timelineTicks * pxPerTick;
    if (_dragActive && dragMode === ArrangerDragState.DragMode.ResizeFromStart)
      return base + dragDeltaPx;
    return base;
  }
  required property real pxPerTick

  // Content offset for normal (Bounds) resize from start. Non-zero only when
  // the left edge is being dragged and this is NOT a loop-resize. Loop resize
  // keeps content in place; normal resize clips it from the left.
  readonly property real resizeContentOffset: (_dragActive && dragMode === ArrangerDragState.DragMode.ResizeFromStart && !isLoopResize) ? dragDeltaPx : 0
  required property real scrollViewWidth
  required property real scrollX
  readonly property alias selectionTracker: selectionTracker

  // The width this loader would have without any drag override. Clip views
  // bind this to their canvas item's referenceWidth so content density stays
  // constant during resize drag.
  readonly property real undraggedWidth: arrangerObject.length ? Math.max((posTracker.timelineEndTicks - posTracker.timelineTicks) * pxPerTick, 2) : 0
  required property UnifiedProxyModel unifiedObjectsModel
  property bool useCustomWidth: false
  readonly property bool useParentPosition: arrangerObject.parentObject !== null

  // Viewport culling
  active: objectEndX + ZrythmTheme.scrollLoaderBufferPx >= scrollX && objectX <= (scrollX + scrollViewWidth + ZrythmTheme.scrollLoaderBufferPx)
  asynchronous: true
  visible: status === Loader.Ready
  x: objectX - (centeredOnPosition ? width / 2 : 0)

  // Move: GPU Translate (zero re-rendering). Resize handled via width/x
  // override above + Binding below — content clips/extends without stretching.
  transform: Translate {
    x: (root._dragActive && root.dragMode === ArrangerDragState.DragMode.Move) ? root.dragDeltaPx : 0
    y: (root._dragActive && root.dragMode === ArrangerDragState.DragMode.Move) ? root.dragDeltaY : 0
  }

  // Width: overridden during resize to clip/extend the delegate. Canvas items
  // inside use referenceWidth (constant) for density so content doesn't stretch.
  Binding on width {
    value: {
      const base = Math.max((posTracker.timelineEndTicks - posTracker.timelineTicks) * root.pxPerTick, 2);
      if (!root.arrangerObject.length)
        return 2;
      if (root.selectionTracker.isSelected) {
        if (root.dragMode === ArrangerDragState.DragMode.ResizeFromEnd)
          return Math.max(base + root.dragDeltaPx, 2);
        if (root.dragMode === ArrangerDragState.DragMode.ResizeFromStart)
          return Math.max(base - root.dragDeltaPx, 2);
      }
      return base;
    }
    when: !root.useCustomWidth && root.arrangerObject.length
  }

  TimelinePositionTracker {
    id: posTracker

    arrangerObject: root.arrangerObject
  }

  SelectionTracker {
    id: selectionTracker

    modelIndexProvider: () => root.unifiedObjectsModel.mapFromSource(root.model.index(root.index, 0))
    selectionModel: root.arrangerSelectionModel
  }
}
