// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import Zrythm

TestCase {
  id: testCase

  name: "ModularMinimap"

  ModularMinimap {
    id: minimap

    editorBodyHeight: 56
    headerHeight: 40
    height: 100
    nodeModel: ListModel {
      id: nodes

      ListElement {
        accentKind: "neutral"
        inputsJson: "[]"
        modulatorDescriptorId: ""
        nodeId: "node-a"
        nodeX: 0
        nodeY: 0
        outputsJson: "[]"
        parametersJson: "[]"
      }
    }
    nodeWidth: 236
    panX: 0
    panY: 0
    portHeight: 26
    selectedNodeId: ""
    viewportHeight: 200
    viewportWidth: 200
    width: 150
    zoom: 1
  }

  function init(): void {
    if (nodes.count > 1)
      nodes.remove(1, nodes.count - 1);
    nodes.setProperty(0, "nodeX", 0);
    nodes.setProperty(0, "nodeY", 0);
  }

  function test_contentRectCoversViewportAndNodes(): void {
    verify(minimap.contentRect.width >= minimap.viewportWidth);
    verify(minimap.contentRect.height >= minimap.viewportHeight);
    verify(minimap.contentRect.x <= 0);
    verify(minimap.contentRect.y <= 0);
  }

  // Node moves made through setProperty() arrive as dataChanged; the extent
  // must follow them even though role values read via ListModel::get() are
  // not visible to bindings.
  function test_contentRectFollowsNodeMoves(): void {
    // Snapshot as numbers: reading the rect property yields a live
    // value-type wrapper, not a copy.
    const beforeX = minimap.contentRect.x;
    const beforeWidth = minimap.contentRect.width;
    nodes.setProperty(0, "nodeX", -2000);
    compare(minimap.nodesBounds.x, -2000);
    compare(minimap.nodesBounds.width, minimap.nodeWidth);
    compare(minimap.contentRect.x, beforeX - 2000);
    verify(minimap.contentRect.width > beforeWidth);
  }

  function test_contentRectFollowsNodeInsertion(): void {
    const before = minimap.contentRect.width;
    nodes.append({
        "accentKind": "neutral",
        "inputsJson": "[]",
        "modulatorDescriptorId": "",
        "nodeId": "node-b",
        "nodeX": 5000,
        "nodeY": 0,
        "outputsJson": "[]",
        "parametersJson": "[]"
      });
    compare(minimap.contentRect.width, before + 5000);
  }
}
