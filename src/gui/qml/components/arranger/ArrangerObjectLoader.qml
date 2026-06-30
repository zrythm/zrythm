// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui

Loader {
  id: root

  required property ArrangerObject arrangerObject
  required property ItemSelectionModel arrangerSelectionModel
  required property int index
  required property var model
  readonly property real objectEndX: x + width

  TimelinePositionTracker {
    id: posTracker
    arrangerObject: root.arrangerObject
  }

  readonly property real objectX: posTracker.timelineTicks * pxPerTick
  required property real pxPerTick
  required property real scrollViewWidth
  required property real scrollX
  readonly property alias selectionTracker: selectionTracker
  required property UnifiedProxyModel unifiedObjectsModel
  property bool useCustomWidth: false // use `width` instead of bounds-based
  readonly property bool useParentPosition: arrangerObject.parentObject !== null

  // Viewport culling
  active: objectEndX + ZrythmTheme.scrollLoaderBufferPx >= scrollX && objectX <= (scrollX + scrollViewWidth + ZrythmTheme.scrollLoaderBufferPx)
  asynchronous: true
  visible: status === Loader.Ready
  x: objectX

  Binding on width {
    value: Math.max((posTracker.timelineEndTicks - posTracker.timelineTicks) * root.pxPerTick, 2)
    when: !root.useCustomWidth && root.arrangerObject.length
  }

  SelectionTracker {
    id: selectionTracker

    // Dummy property to force the modelIndex to be recalculated
    property bool dummy: false

    modelIndex: {
      root.unifiedObjectsModel.addSourceModel(root.model);
      const ret = root.unifiedObjectsModel.mapFromSource(root.model.index(root.index, 0));
      dummy;
      return ret;
    }
    selectionModel: root.arrangerSelectionModel
  }

  // When objects are added to previous source models in the unified model, the modelIndex above is not updated, so we force an update here when objects are added/removed
  Connections {
    function onRowsInserted() {
      selectionTracker.dummy = !selectionTracker.dummy;
    }

    function onRowsRemoved() {
      selectionTracker.dummy = !selectionTracker.dummy;
    }

    target: root.unifiedObjectsModel
  }
}
