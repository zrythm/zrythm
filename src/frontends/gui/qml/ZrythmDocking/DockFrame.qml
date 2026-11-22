// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: root

  required property DockingController controller
  required property var node
  property string dropZone: ""
  property string registeredFrameId: ""

  implicitHeight: 160
  implicitWidth: 200

  function dragPanelId(drag: var): string {
    if (drag.formats.indexOf(controller.panelMimeType) < 0)
      return "";
    try {
      const payload = JSON.parse(drag.getDataAsString(controller.panelMimeType));
      if (payload.workspaceId !== controller.workspaceId || controller.panelById(payload.panelId) === null)
        return "";
      return payload.panelId;
    } catch (error) {
      return "";
    }
  }

  function dropZoneAt(x: real, y: real): string {
    if (y < 36 || (x > width / 4 && x < width * 3 / 4 && y > height / 4 && y < height * 3 / 4))
      return "tab";
    if (x < width / 4)
      return "left";
    if (x > width * 3 / 4)
      return "right";
    if (y < height / 2)
      return "top";
    return "bottom";
  }

  function validDropZone(panelId: string, x: real, y: real): string {
    if (node === null)
      return "";
    const zone = dropZoneAt(x, y);
    return controller.canMovePanel(panelId, node.id, zone) ? zone : "";
  }

  function registerHost(): void {
    if (node === null || activePanelHost === null)
      return;
    if (registeredFrameId.length > 0 && registeredFrameId !== node.id)
      controller.unregisterFrameHost(registeredFrameId, activePanelHost);
    registeredFrameId = node.id;
    controller.registerFrameHost(registeredFrameId, activePanelHost);
  }

  function tabIndexAt(x: real): int {
    if (node === null)
      return 0;
    if (node.panels.length < 2)
      return node.panels.length;
    return Math.max(0, Math.min(node.panels.length, Math.floor(x / width * node.panels.length)));
  }

  onNodeChanged: registerHost()

  Component.onCompleted: registerHost()
  Component.onDestruction: {
    if (registeredFrameId.length > 0)
      controller.unregisterFrameHost(registeredFrameId, activePanelHost);
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    Loader {
      active: root.node?.headerVisible !== false && root.node?.tabsAtBottom !== true
      Layout.fillWidth: true
      Layout.preferredHeight: active && status === Loader.Ready ? (item as TabBar).implicitHeight : 0
      sourceComponent: tabBarComponent
      visible: active
    }

    Item {
      id: activePanelHost

      Layout.fillHeight: true
      Layout.fillWidth: true
      Layout.minimumHeight: 60
      Layout.minimumWidth: 80
    }

    Loader {
      active: root.node?.headerVisible !== false && root.node?.tabsAtBottom === true
      Layout.fillWidth: true
      Layout.preferredHeight: active && status === Loader.Ready ? (item as TabBar).implicitHeight : 0
      sourceComponent: tabBarComponent
      visible: active
    }
  }

  Component {
    id: tabBarComponent

    TabBar {
      id: tabBar

      currentIndex: Math.max(0, root.node.panels.indexOf(root.node.active))
      implicitHeight: 36

      Repeater {
        model: root.node.panels

        delegate: TabButton {
          id: tabButton

          required property string modelData
          property DockPanel panel: null

          function updatePanel(): void {
            panel = root.controller.panelById(modelData);
          }

          icon.source: panel !== null ? panel.icon : ""
          text: root.node.showLabels !== false && panel !== null ? panel.title : ""

          onClicked: root.controller.activatePanelInFrame(root.node.id, modelData)
          onModelDataChanged: updatePanel()

          Component.onCompleted: updatePanel()

          Connections {
            function onPanelRegistryChanged() {
              tabButton.updatePanel();
            }

            target: root.controller
          }

          DragHandler {
            id: dragHandler

            target: null

            onActiveChanged: {
              const panel = tabButton.panel;
              if (active)
                root.controller.draggedPanelId = tabButton.modelData;
              if (panel === null) {
                if (!active)
                  root.controller.draggedPanelId = "";
                return;
              }
              if (!active) {
                panel.dragActive = false;
                if (!panel.Drag.active && root.controller.draggedPanelId === tabButton.modelData)
                  root.controller.draggedPanelId = "";
                return;
              }
              tabButton.grabToImage(function(result) {
                if (dragHandler.active) {
                  panel.Drag.imageSource = result.url;
                  panel.dragActive = true;
                }
              });
            }
          }

          TapHandler {
            acceptedButtons: Qt.RightButton

            onTapped: tabMenu.popup()
          }

          ToolTip.delay: 500
          ToolTip.text: panel !== null ? panel.title : ""
          ToolTip.visible: hovered

          Menu {
            id: tabMenu

            MenuItem {
              enabled: tabButton.panel !== null && root.controller.canDetachPanel(tabButton.modelData)
              text: qsTr("Move to New Window")

              onTriggered: root.controller.detachPanel(tabButton.modelData)
            }

            MenuItem {
              enabled: tabButton.panel !== null && tabButton.panel.closable
              text: qsTr("Close Panel")

              onTriggered: root.controller.hidePanel(tabButton.modelData)
            }
          }
        }
      }
    }
  }

  DropArea {
    id: dropArea

    anchors.fill: parent
    keys: [root.controller.panelMimeType]
    visible: root.controller.draggedPanelId.length > 0

    onEntered: drag => {
      const panelId = root.dragPanelId(drag);
      root.dropZone = panelId.length > 0 ? root.validDropZone(panelId, drag.x, drag.y) : "";
      drag.accepted = root.dropZone.length > 0;
    }
    onExited: root.dropZone = ""
    onPositionChanged: drag => {
      const panelId = root.dragPanelId(drag);
      root.dropZone = panelId.length > 0 ? root.validDropZone(panelId, drag.x, drag.y) : "";
      drag.accepted = root.dropZone.length > 0;
    }
    onDropped: drop => {
      const panelId = root.dragPanelId(drop);
      const zone = panelId.length > 0 ? root.validDropZone(panelId, drop.x, drop.y) : "";
      if (zone.length > 0 && root.controller.movePanel(panelId, root.node.id, zone, root.tabIndexAt(drop.x)))
        drop.accept(Qt.MoveAction);
      else
        drop.accepted = false;
      root.dropZone = "";
      root.controller.draggedPanelId = "";
    }
  }

  DockDropOverlay {
    accentColor: root.controller.dockPalette.highlight
    anchors.fill: parent
    zone: dropArea.containsDrag ? root.dropZone : ""
  }
}
