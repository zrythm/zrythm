// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
  id: root

  required property DockingController controller
  readonly property var floatingState: controller.floatingState(windowId)
  readonly property string activePanelTitle: panelTitle(floatingState !== null ? floatingState.root : null)
  required property string windowId

  function applyFloatingGeometry(): void {
    if (floatingState === null)
      return;
    if (x !== floatingState.x)
      x = floatingState.x;
    if (y !== floatingState.y)
      y = floatingState.y;
    if (width !== floatingState.width)
      width = floatingState.width;
    if (height !== floatingState.height)
      height = floatingState.height;
  }

  function panelTitle(node: var): string {
    if (node === null)
      return "";
    if (node.kind === "frame") {
      const panel = controller.panelById(node.active);
      return panel !== null ? panel.title : "";
    }
    for (const child of node.children) {
      const title = panelTitle(child);
      if (title.length > 0)
        return title;
    }
    return "";
  }

  color: palette.window
  minimumHeight: 140
  minimumWidth: 200
  title: activePanelTitle.length > 0 ? activePanelTitle : qsTr("Panels")
  visible: floatingState !== null

  onActiveChanged: {
    if (active)
      controller.focusItemChanged(activeFocusItem);
  }
  onActiveFocusItemChanged: controller.focusItemChanged(activeFocusItem)
  onClosing: close => {
    close.accepted = false;
    Qt.callLater(() => controller.closeFloating(windowId));
  }
  onFloatingStateChanged: applyFloatingGeometry()
  onHeightChanged: geometryTimer.restart()
  onWidthChanged: geometryTimer.restart()
  onXChanged: geometryTimer.restart()
  onYChanged: geometryTimer.restart()

  Component.onCompleted: {
    applyFloatingGeometry();
    const screenLeft = Screen.virtualX;
    const screenTop = Screen.virtualY;
    const outside = x + width < screenLeft || y + height < screenTop || x > screenLeft + Screen.width || y > screenTop + Screen.height;
    if (outside)
      controller.updateFloatingGeometry(windowId, screenLeft + 80, screenTop + 80, width, height);
  }

  Timer {
    id: geometryTimer

    interval: 200

    onTriggered: {
      if (root.floatingState !== null)
        root.controller.updateFloatingGeometry(root.windowId, root.x, root.y, root.width, root.height);
    }
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    ToolBar {
      Layout.fillWidth: true

      RowLayout {
        anchors.fill: parent

        Item {
          Layout.fillWidth: true
        }

        ToolButton {
          text: qsTr("Return to Dock")

          onClicked: root.controller.closeFloating(root.windowId)
        }
      }
    }

    DockNode {
      Layout.fillHeight: true
      Layout.fillWidth: true
      controller: root.controller
      node: root.floatingState !== null ? root.floatingState.root : null
    }
  }
}
