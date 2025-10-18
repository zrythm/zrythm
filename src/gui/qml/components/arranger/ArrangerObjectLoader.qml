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
    value: root.arrangerObject.bounds.length.ticks * root.pxPerTick
    when: root.arrangerObject.bounds
  }

  SelectionTracker {
    id: selectionTracker

    modelIndex: {
      root.unifiedObjectsModel.addSourceModel(root.model);
      return root.unifiedObjectsModel.mapFromSource(root.model.index(root.index, 0));
    }
    selectionModel: root.arrangerSelectionModel
  }
}
