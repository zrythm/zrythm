// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

Loader {
  id: root

  required property ArrangerObject arrangerObject
  required property ItemSelectionModel arrangerSelectionModel
  required property int index
  required property var model
  readonly property real objectEndX: x + width
  readonly property real objectX: {
    const objectTimelineTicks = arrangerObject.position.ticks + (arrangerObject.parentObject ? arrangerObject.parentObject.position.ticks : 0);
    return objectTimelineTicks * pxPerTick;
  }
  required property real pxPerTick
  required property real scrollViewWidth
  required property real scrollX
  readonly property alias selectionTracker: selectionTracker
  required property UnifiedArrangerObjectsModel unifiedObjectsModel
  readonly property bool useParentPosition: arrangerObject.parentObject !== null

  // Viewport culling
  active: objectEndX + Style.scrollLoaderBufferPx >= scrollX && objectX <= (scrollX + scrollViewWidth + Style.scrollLoaderBufferPx)
  asynchronous: true
  visible: status === Loader.Ready
  x: objectX

  Binding on width {
    value: root.arrangerObject.bounds ? root.arrangerObject.bounds.length.ticks * root.pxPerTick : 0
    when: root.arrangerObject.bounds
  }

  SelectionTracker {
    id: selectionTracker

    // Dummy property to force the unifiedModelIndex to be recalculated
    property bool dummy: false

    selectionModel: root.arrangerSelectionModel
    unifiedModelIndex: {
      root.unifiedObjectsModel.addSourceModel(root.model);
      const ret = root.unifiedObjectsModel.mapFromSource(root.model.index(root.index, 0));
      dummy;
      return ret;
    }
  }

  // When objects are added to previous source models in the unified model, the unifiedModelIndex above is not updated, so we force an update here when objects are added/removed
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
