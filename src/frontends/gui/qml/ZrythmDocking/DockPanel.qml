// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick

Item {
  id: root

  property bool closable: true
  property string defaultFrame: "center"
  property var dockingController: null
  property bool dragActive: false
  property string dragMimeType: ""
  property bool floatable: true
  property url icon
  property bool movable: true
  property Item panelHost: null
  required property string panelId
  property Item storageHost: null
  property string title
  property string workspaceId: ""

  anchors.fill: parent
  opacity: panelHost !== null ? 1 : 0
  visible: panelHost !== null

  Behavior on opacity {
    OpacityAnimator {
      duration: 120
    }
  }

  Drag.active: root.dragActive
  Drag.dragType: Drag.Automatic
  Drag.keys: [root.dragMimeType]
  Drag.mimeData: ({
    [root.dragMimeType]: JSON.stringify({
      "workspaceId": root.workspaceId,
      "panelId": root.panelId
    })
  })
  Drag.supportedActions: Qt.MoveAction
  Drag.onDragFinished: {
    if (root.dockingController !== null && root.dockingController.draggedPanelId === root.panelId)
      root.dockingController.draggedPanelId = "";
  }
  Drag.onActiveChanged: {
    if (!Drag.active)
      root.dragActive = false;
  }

  Binding {
    target: root
    property: "parent"
    value: root.panelHost !== null ? root.panelHost : root.storageHost
    when: root.storageHost !== null
  }
}
