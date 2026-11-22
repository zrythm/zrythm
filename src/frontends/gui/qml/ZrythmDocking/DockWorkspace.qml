// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
  id: root

  default property alias panels: panelStorage.data
  property var defaultLayout: null
  required property Palette dockPalette
  readonly property DockingController dockingController: controller
  property bool bottomRevealed: true
  property bool endRevealed: true
  property string initialLayout: ""
  property bool startRevealed: true
  property bool topRevealed: true
  property string workspaceId: ""

  signal areaRevealRequested(string area)

  Component.onCompleted: {
    if (workspaceId.length === 0)
      workspaceId = "workspace-" + Date.now().toString(36) + "-" + Math.floor(Math.random() * 0x7fffffff).toString(36);
    controller.workspaceId = workspaceId;
    controller.mainWindow = root.Window.window;
    controller.floatingWindowComponent = floatingWindowComponent;
    controller.defaultLayout = defaultLayout;
    controller.resetLayout();
    // Registration can reparent panels, so iterate over a stable snapshot.
    const panels = panelStorage.data.slice();
    for (const object of panels) {
      if (object.panelId !== undefined) {
        object.storageHost = panelStorage;
        controller.registerPanel(object);
      }
    }
    if (initialLayout.length > 0)
      controller.restoreState(initialLayout);
  }
  Component.onDestruction: controller.shutdown()

  DockingController {
    id: controller

    dockPalette: root.dockPalette
  }

  Item {
    id: panelStorage

    height: 0
    visible: false
    width: 0
  }

  Component {
    id: floatingWindowComponent

    DockWindow {
      transientParent: root.Window.window

      Component.onCompleted: palette = Qt.binding(() => root.dockPalette)
    }
  }

  Connections {
    function onAreaRevealRequested(area: string) {
      root.areaRevealRequested(area);
    }

    target: controller
  }

  SplitView {
    id: outerSplit

    anchors.fill: parent
    orientation: Qt.Horizontal

    onResizingChanged: {
      if (!resizing) {
        if (startLoader.active && root.startRevealed && controller.layout.main.areas.start !== null)
          controller.setAreaSize("start", startLoader.width);
        if (endLoader.active && root.endRevealed && controller.layout.main.areas.end !== null)
          controller.setAreaSize("end", endLoader.width);
      }
    }

    Loader {
      id: startLoader

      active: true
      visible: root.startRevealed && (controller.layout.main.areas.start !== null || controller.canMovePanelToArea(controller.draggedPanelId, "start"))
      SplitView.fillHeight: true
      SplitView.minimumWidth: visible ? (controller.layout.main.areas.start === null ? 32 : 120) : 0
      SplitView.preferredWidth: visible ? (controller.layout.main.areas.start === null ? 48 : controller.layout.main.sizes.start) : 0
      sourceComponent: DockNode {
        anchors.fill: parent
        area: "start"
        controller: root.dockingController
        node: root.dockingController.layout.main.areas.start
      }
    }

    SplitView {
      id: centerSplit

      SplitView.fillHeight: true
      SplitView.fillWidth: true
      SplitView.minimumWidth: 160
      orientation: Qt.Vertical

      onResizingChanged: {
        if (!resizing) {
          if (topLoader.active && root.topRevealed && controller.layout.main.areas.top !== null)
            controller.setAreaSize("top", topLoader.height);
          if (bottomLoader.active && root.bottomRevealed && controller.layout.main.areas.bottom !== null)
            controller.setAreaSize("bottom", bottomLoader.height);
        }
      }

      Loader {
        id: topLoader

        active: true
        visible: root.topRevealed && (controller.layout.main.areas.top !== null || controller.canMovePanelToArea(controller.draggedPanelId, "top"))
        SplitView.fillWidth: true
        SplitView.minimumHeight: visible ? (controller.layout.main.areas.top === null ? 32 : 80) : 0
        SplitView.preferredHeight: visible ? (controller.layout.main.areas.top === null ? 48 : controller.layout.main.sizes.top) : 0
        sourceComponent: DockNode {
          anchors.fill: parent
          area: "top"
          controller: root.dockingController
          node: root.dockingController.layout.main.areas.top
        }
      }

      Loader {
        id: centerLoader

        SplitView.fillHeight: true
        SplitView.fillWidth: true
        SplitView.minimumHeight: 100
        active: true
        sourceComponent: DockNode {
          anchors.fill: parent
          area: "center"
          controller: root.dockingController
          node: root.dockingController.layout.main.areas.center
        }
      }

      Loader {
        id: bottomLoader

        active: true
        visible: root.bottomRevealed && (controller.layout.main.areas.bottom !== null || controller.canMovePanelToArea(controller.draggedPanelId, "bottom"))
        SplitView.fillWidth: true
        SplitView.minimumHeight: visible ? (controller.layout.main.areas.bottom === null ? 32 : 80) : 0
        SplitView.preferredHeight: visible ? (controller.layout.main.areas.bottom === null ? 48 : controller.layout.main.sizes.bottom) : 0
        sourceComponent: DockNode {
          anchors.fill: parent
          area: "bottom"
          controller: root.dockingController
          node: root.dockingController.layout.main.areas.bottom
        }
      }
    }

    Loader {
      id: endLoader

      active: true
      visible: root.endRevealed && (controller.layout.main.areas.end !== null || controller.canMovePanelToArea(controller.draggedPanelId, "end"))
      SplitView.fillHeight: true
      SplitView.minimumWidth: visible ? (controller.layout.main.areas.end === null ? 32 : 120) : 0
      SplitView.preferredWidth: visible ? (controller.layout.main.areas.end === null ? 48 : controller.layout.main.sizes.end) : 0
      sourceComponent: DockNode {
        anchors.fill: parent
        area: "end"
        controller: root.dockingController
        node: root.dockingController.layout.main.areas.end
      }
    }
  }

  Rectangle {
    id: detachTarget

    readonly property bool candidate: controller.draggedPanelId.length > 0 && controller.canDetachPanel(controller.draggedPanelId)

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: parent.top
    anchors.topMargin: 12
    border.color: root.dockPalette.highlight
    border.width: 1
    color: root.dockPalette.alternateBase
    height: 48
    opacity: candidate ? 1 : 0
    radius: 4
    scale: candidate ? 1 : 0.95
    visible: opacity > 0
    width: 180
    z: 100

    Behavior on opacity {
      OpacityAnimator {
        duration: 120
      }
    }
    Behavior on scale {
      ScaleAnimator {
        duration: 120
      }
    }

    Label {
      anchors.centerIn: parent
      text: qsTr("Drop to Detach")
    }

    DropArea {
      anchors.fill: parent
      keys: [controller.panelMimeType]

      onDropped: drop => {
        try {
          const payload = JSON.parse(drop.getDataAsString(controller.panelMimeType));
          if (payload.workspaceId === controller.workspaceId && controller.detachPanel(payload.panelId))
            drop.accept(Qt.MoveAction);
          else
            drop.accepted = false;
        } catch (error) {
          drop.accepted = false;
        }
        controller.draggedPanelId = "";
      }
    }
  }
}
