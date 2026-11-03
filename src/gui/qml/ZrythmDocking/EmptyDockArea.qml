// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls

Rectangle {
  id: root

  required property string area
  required property DockingController controller

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

  border.color: dropArea.containsDrag ? controller.dockPalette.highlight : controller.dockPalette.mid
  border.width: dropArea.containsDrag ? 2 : 1
  color: dropArea.containsDrag ? Qt.rgba(controller.dockPalette.highlight.r, controller.dockPalette.highlight.g, controller.dockPalette.highlight.b, 0.25) : controller.dockPalette.alternateBase
  radius: 4

  Behavior on border.color {
    ColorAnimation {
      duration: 100
    }
  }
  Behavior on color {
    ColorAnimation {
      duration: 100
    }
  }

  Label {
    anchors.centerIn: parent
    color: root.controller.dockPalette.windowText
    font.pixelSize: compact ? 20 : 13
    text: compact ? "+" : qsTr("Drop Panels Here")

    readonly property bool compact: root.width < 100 || root.height < 48
  }

  DropArea {
    id: dropArea

    anchors.fill: parent
    keys: [root.controller.panelMimeType]

    onEntered: drag => {
      const panelId = root.dragPanelId(drag);
      drag.accepted = panelId.length > 0 && root.controller.canMovePanelToArea(panelId, root.area);
    }
    onDropped: drop => {
      const panelId = root.dragPanelId(drop);
      if (panelId.length > 0 && root.controller.canMovePanelToArea(panelId, root.area) && root.controller.movePanelToArea(panelId, root.area))
        drop.accept(Qt.MoveAction);
      else
        drop.accepted = false;
      root.controller.draggedPanelId = "";
    }
  }
}
