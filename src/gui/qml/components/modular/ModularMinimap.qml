// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmStyle

Item {
  id: root

  required property int headerHeight
  required property var nodeModel
  required property int nodeWidth
  required property real panX
  required property real panY
  required property int portHeight
  required property string selectedNodeId
  required property real viewportHeight
  required property real viewportWidth
  required property real zoom

  // Node extents. Recomputed on model changes: role values read through
  // ListModel::get() are not visible to bindings, while the delegates' own
  // role bindings follow drags.
  property rect nodesBounds: Qt.rect(0, 0, 0, 0)

  function recomputeNodesBounds(): void {
    if (nodeModel === null) {
      nodesBounds = Qt.rect(0, 0, 0, 0);
      return;
    }
    let minX = Infinity;
    let minY = Infinity;
    let maxX = -Infinity;
    let maxY = -Infinity;
    for (let i = 0; i < nodeModel.count; ++i) {
      const node = nodeModel.get(i);
      minX = Math.min(minX, node.nodeX);
      minY = Math.min(minY, node.nodeY);
      maxX = Math.max(maxX, node.nodeX + nodeWidth);
      maxY = Math.max(maxY, node.nodeY + nodeHeight(node));
    }
    if (minX === Infinity) {
      nodesBounds = Qt.rect(0, 0, 0, 0);
      return;
    }
    nodesBounds = Qt.rect(minX, minY, maxX - minX, maxY - minY);
  }

  // Union of the viewport rectangle and the node extents.
  readonly property rect contentRect: {
    const margin = 40;
    let minX = -panX / zoom;
    let minY = -panY / zoom;
    let maxX = minX + viewportWidth / zoom;
    let maxY = minY + viewportHeight / zoom;
    if (nodesBounds.width > 0) {
      minX = Math.min(minX, nodesBounds.x);
      minY = Math.min(minY, nodesBounds.y);
      maxX = Math.max(maxX, nodesBounds.x + nodesBounds.width);
      maxY = Math.max(maxY, nodesBounds.y + nodesBounds.height);
    }
    return Qt.rect(minX - margin, minY - margin, maxX - minX + margin * 2, maxY - minY + margin * 2);
  }
  readonly property real mapScale: Math.max(0.0001, Math.min((width - 16) / contentRect.width, (height - 16) / contentRect.height))

  signal navigationRequested(point worldCenter)

  function navigateTo(localPoint: point) {
    navigationRequested(Qt.point(contentRect.x + (localPoint.x - 8) / mapScale, contentRect.y + (localPoint.y - 8) / mapScale));
  }

  function nodeHeight(node: var): real {
    const inputs = JSON.parse(node.inputsJson);
    const outputs = JSON.parse(node.outputsJson);
    const parameters = JSON.parse(node.parametersJson);
    return headerHeight + Math.max(inputs.length + parameters.length, outputs.length) * portHeight + 8;
  }

  Component.onCompleted: recomputeNodesBounds()
  onNodeModelChanged: recomputeNodesBounds()

  Connections {
    target: root.nodeModel

    function onDataChanged() {
      root.recomputeNodesBounds();
    }
    function onModelReset() {
      root.recomputeNodesBounds();
    }
    function onRowsInserted() {
      root.recomputeNodesBounds();
    }
    function onRowsRemoved() {
      root.recomputeNodesBounds();
    }
  }

  Accessible.name: qsTr("Graph minimap. Drag to navigate.")
  Accessible.role: Accessible.Button
  activeFocusOnTab: true

  Rectangle {
    anchors.fill: parent
    border.color: ZrythmTheme.alternateBackgroundColor
    border.width: 1
    color: Qt.alpha(ZrythmTheme.cardColor, 0.96)
    radius: 6
  }

  Item {
    id: mapContent

    anchors.fill: parent
    clip: true

    Repeater {
      model: root.nodeModel

      delegate: Rectangle {
        required property string accentKind
        required property string inputsJson
        required property string nodeId
        required property real nodeX
        required property real nodeY
        required property string outputsJson
        required property string parametersJson

        color: root.selectedNodeId === nodeId ? ZrythmTheme.primaryColor : ZrythmTheme.placeholderTextColor
        height: Math.max(2, root.nodeHeight({
              "inputsJson": inputsJson,
              "outputsJson": outputsJson,
              "parametersJson": parametersJson
            }) * root.mapScale)
        width: Math.max(2, root.nodeWidth * root.mapScale)
        x: 8 + (nodeX - root.contentRect.x) * root.mapScale
        y: 8 + (nodeY - root.contentRect.y) * root.mapScale
      }
    }

    Rectangle {
      border.color: ZrythmTheme.textColor
      border.width: 1
      color: Qt.alpha(ZrythmTheme.textColor, 0.08)
      height: Math.max(4, root.viewportHeight / root.zoom * root.mapScale)
      width: Math.max(4, root.viewportWidth / root.zoom * root.mapScale)
      x: 8 + (-root.panX / root.zoom - root.contentRect.x) * root.mapScale
      y: 8 + (-root.panY / root.zoom - root.contentRect.y) * root.mapScale
    }
  }

  DragHandler {
    id: navigationHandler

    acceptedButtons: Qt.LeftButton
    dragThreshold: 0
    grabPermissions: PointerHandler.CanTakeOverFromAnything | PointerHandler.ApprovesCancellation
    target: null

    onActiveChanged: {
      if (active)
        root.navigateTo(centroid.position);
    }
    onActiveTranslationChanged: {
      if (active)
        root.navigateTo(centroid.position);
    }
  }
}
