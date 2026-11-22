// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import "DockingLayout.js" as Layout

QtObject {
  id: root

  property var defaultLayout: null
  required property Palette dockPalette
  property string draggedPanelId: ""
  property Component floatingWindowComponent: null
  readonly property int floatingWindowCount: layout.floating.length
  property var layout: Layout.emptyLayout()
  property var mainWindow: null
  property var panelIds: []
  readonly property string panelMimeType: "application/x-zrythm-dock-panel"
  property string workspaceId: ""

  property var _frameHosts: ({})
  property int _nextId: 1
  property var _panels: []
  property bool _shuttingDown: false
  property var _windows: ({})

  signal areaRevealRequested(string area)
  signal floatingWindowCreated(var window)
  signal focusItemChanged(Item focusItem)
  signal panelRegistryChanged()

  function activatePanelInFrame(frameId: string, panelId: string): void {
    const location = Layout.findPanel(layout, panelId);
    if (location === null || location.frameId !== frameId)
      return;
    const frame = Layout.findFrameInLayout(layout, frameId);
    if (frame === null || frame.active === panelId)
      return;
    const next = Layout.clone(layout);
    Layout.findFrameInLayout(next, frameId).active = panelId;
    commit(next);
  }

  function canDetachPanel(panelId: string): bool {
    const panel = panelById(panelId);
    const location = Layout.findPanel(layout, panelId);
    return panel !== null && location !== null && panel.floatable && location.windowId === null;
  }

  function canMovePanel(panelId: string, targetFrameId: string, zone: string): bool {
    if (zone === "newWindow")
      return canDetachPanel(panelId);
    const panel = panelById(panelId);
    const source = Layout.findPanel(layout, panelId);
    const target = Layout.findFrameInLayout(layout, targetFrameId);
    if (panel === null || source === null || target === null || !panel.movable)
      return false;
    if (zone !== "tab" && zone !== "left" && zone !== "right" && zone !== "top" && zone !== "bottom")
      return false;
    if (source.frameId === targetFrameId && zone !== "tab")
      return target.panels.length > 1;
    return true;
  }

  function canMovePanelToArea(panelId: string, area: string): bool {
    const panel = panelById(panelId);
    const source = Layout.findPanel(layout, panelId);
    return panel !== null && source !== null && panel.movable && Layout.areaNames().indexOf(area) >= 0 && layout.main.areas[area] === null;
  }

  function closeFloating(windowId: string): bool {
    if (_shuttingDown)
      return false;
    const next = Layout.clone(layout);
    const index = next.floating.findIndex(floating => floating.id === windowId);
    if (index < 0)
      return false;

    const floating = next.floating[index];
    const panelIdsToRestore = [];
    Layout.collectPanelIds(floating.root, panelIdsToRestore);
    next.floating.splice(index, 1);
    for (const panelId of panelIdsToRestore) {
      const panel = panelById(panelId);
      const preferredFrame = floating.returnFrames && floating.returnFrames[panelId] ? floating.returnFrames[panelId] : (panel !== null ? panel.defaultFrame : "center");
      ensurePanelInMain(next, panelId, preferredFrame);
      next.hidden = next.hidden.filter(id => id !== panelId);
    }
    commit(next);
    return true;
  }

  function commit(next: var): void {
    if (_shuttingDown)
      return;
    layout = next;
    syncWindows();
    syncPanelHosts();
  }

  function detachPanel(panelId: string): bool {
    if (!canDetachPanel(panelId))
      return false;
    const location = Layout.findPanel(layout, panelId);

    const next = Layout.clone(layout);
    Layout.removePanel(next, panelId);
    const frameId = uniqueNodeId("floating-frame");
    const windowId = uniqueWindowId();
    const offset = 40 + (next.floating.length % 8) * 24;
    const returnFrames = ({});
    returnFrames[panelId] = location.frameId;
    next.floating.push({
      "id": windowId,
      "root": Layout.makeFrame(frameId, [panelId], panelId),
      "returnFrames": returnFrames,
      "x": (mainWindow !== null ? mainWindow.x : 80) + offset,
      "y": (mainWindow !== null ? mainWindow.y : 80) + offset,
      "width": 480,
      "height": 360
    });
    next.hidden = next.hidden.filter(id => id !== panelId);
    commit(next);
    return true;
  }

  function ensurePanelInMain(targetLayout: var, panelId: string, preferredFrameId: string): void {
    if (Layout.findPanel(targetLayout, panelId) !== null)
      return;
    const panel = panelById(panelId);
    const defaultFrameId = panel !== null ? panel.defaultFrame : preferredFrameId;
    if (Layout.insertIntoFrame(targetLayout, preferredFrameId, panelId, undefined))
      return;
    if (defaultFrameId !== preferredFrameId && Layout.insertIntoFrame(targetLayout, defaultFrameId, panelId, undefined))
      return;
    const fallback = Layout.firstMainFrame(targetLayout);
    if (fallback !== null) {
      Layout.insertIntoFrame(targetLayout, fallback.id, panelId, undefined);
      return;
    }
    targetLayout.main.areas.center = Layout.makeFrame(uniqueNodeId("center"), [panelId], panelId);
  }

  function floatingState(windowId: string): var {
    const floating = layout.floating.find(entry => entry.id === windowId);
    return floating === undefined ? null : floating;
  }

  function hidePanel(panelId: string): bool {
    const panel = panelById(panelId);
    if (panel === null || !panel.closable || Layout.findPanel(layout, panelId) === null)
      return false;
    const next = Layout.clone(layout);
    Layout.removePanel(next, panelId);
    if (next.hidden.indexOf(panelId) < 0)
      next.hidden.push(panelId);
    commit(next);
    return true;
  }

  function isPanelHidden(panelId: string): bool {
    return layout.hidden.indexOf(panelId) >= 0;
  }

  function movePanel(panelId: string, targetFrameId: string, zone: string, tabIndex: int): bool {
    if (zone === "newWindow")
      return detachPanel(panelId);
    if (!canMovePanel(panelId, targetFrameId, zone))
      return false;
    const source = Layout.findPanel(layout, panelId);

    const next = Layout.clone(layout);
    if (source.frameId === targetFrameId && zone === "tab") {
      const frame = Layout.findFrameInLayout(next, targetFrameId);
      const oldIndex = frame.panels.indexOf(panelId);
      frame.panels.splice(oldIndex, 1);
      let adjustedIndex = tabIndex;
      if (oldIndex < tabIndex)
        --adjustedIndex;
      frame.panels.splice(Math.max(0, Math.min(frame.panels.length, adjustedIndex)), 0, panelId);
      frame.active = panelId;
      commit(next);
      return true;
    }

    Layout.removePanel(next, panelId);
    const target = Layout.findFrameInLayout(next, targetFrameId);
    if (target === null)
      return false;

    if (zone === "tab") {
      Layout.insertIntoFrame(next, targetFrameId, panelId, tabIndex);
    } else {
      const horizontal = zone === "left" || zone === "right";
      if (!horizontal && zone !== "top" && zone !== "bottom")
        return false;
      const newFrame = Layout.makeFrame(uniqueNodeId("frame"), [panelId], panelId);
      const before = zone === "left" || zone === "top";
      const split = {
        "id": uniqueNodeId("split"),
        "kind": "split",
        "orientation": horizontal ? "horizontal" : "vertical",
        "children": before ? [newFrame, target] : [target, newFrame],
        "sizes": [0.5, 0.5]
      };
      Layout.replaceNode(next, targetFrameId, split);
    }
    Layout.normalizeLayout(next);

    const destination = Layout.findPanel(next, panelId);
    if (destination !== null && destination.windowId !== null) {
      const floating = next.floating.find(entry => entry.id === destination.windowId);
      if (floating.returnFrames === undefined)
        floating.returnFrames = ({});
      let returnFrame = source.frameId;
      if (source.windowId !== null) {
        const sourceFloating = layout.floating.find(entry => entry.id === source.windowId);
        if (sourceFloating.returnFrames && sourceFloating.returnFrames[panelId])
          returnFrame = sourceFloating.returnFrames[panelId];
      }
      floating.returnFrames[panelId] = returnFrame;
    }
    next.hidden = next.hidden.filter(id => id !== panelId);
    commit(next);
    return true;
  }

  function movePanelToArea(panelId: string, area: string): bool {
    if (!canMovePanelToArea(panelId, area))
      return false;

    const next = Layout.clone(layout);
    Layout.removePanel(next, panelId);
    const frameId = findNode(next, area) === null ? area : uniqueNodeId(area);
    next.main.areas[area] = Layout.makeFrame(frameId, [panelId], panelId);
    next.hidden = next.hidden.filter(id => id !== panelId);
    commit(next);
    return true;
  }

  function panelById(panelId: string): DockPanel {
    for (const panel of _panels) {
      if (panel.panelId === panelId)
        return panel;
    }
    return null;
  }

  function panelLocation(panelId: string): var {
    return Layout.findPanel(layout, panelId);
  }

  function presentPanel(panelId: string): bool {
    const panel = panelById(panelId);
    if (panel === null)
      return false;
    let location = Layout.findPanel(layout, panelId);
    if (location === null) {
      const next = Layout.clone(layout);
      ensurePanelInMain(next, panelId, panel.defaultFrame);
      next.hidden = next.hidden.filter(id => id !== panelId);
      commit(next);
      location = Layout.findPanel(layout, panelId);
    }
    if (location === null)
      return false;
    activatePanelInFrame(location.frameId, panelId);
    if (location.area !== null)
      areaRevealRequested(location.area);
    else if (location.windowId !== null && _windows[location.windowId] !== undefined) {
      _windows[location.windowId].visible = true;
      _windows[location.windowId].requestActivate();
    }
    return true;
  }

  function registerFrameHost(frameId: string, host: Item): void {
    _frameHosts[frameId] = host;
    syncPanelHosts();
  }

  function registerPanel(panel: DockPanel): bool {
    if (panel === null || panelById(panel.panelId) !== null) {
      console.warn("Duplicate dock panel ID:", panel !== null ? panel.panelId : "null");
      return false;
    }
    panel.dockingController = root;
    panel.dragMimeType = panelMimeType;
    panel.workspaceId = workspaceId;
    _panels.push(panel);
    panelIds = _panels.map(item => item.panelId);
    panelRegistryChanged();
    if (Layout.findPanel(layout, panel.panelId) === null && layout.hidden.indexOf(panel.panelId) < 0) {
      const next = Layout.clone(layout);
      ensurePanelInMain(next, panel.panelId, panel.defaultFrame);
      commit(next);
    } else {
      syncPanelHosts();
    }
    return true;
  }

  function resetLayout(): void {
    const next = defaultLayout === null ? Layout.emptyLayout() : Layout.clone(defaultLayout);
    for (const panel of _panels) {
      if (Layout.findPanel(next, panel.panelId) === null)
        ensurePanelInMain(next, panel.panelId, panel.defaultFrame);
    }
    commit(next);
  }

  function restoreState(serialized: string): bool {
    if (serialized.length === 0)
      return false;
    let candidate;
    try {
      candidate = JSON.parse(serialized);
    } catch (error) {
      console.warn("Ignoring malformed dock layout JSON:", error);
      return false;
    }
    if (!isValidLayout(candidate)) {
      console.warn("Ignoring invalid dock layout");
      return false;
    }

    const seenPanels = ({});
    const seenNodes = ({});
    const seenWindows = ({});
    const next = Layout.clone(candidate);
    for (const area of Layout.areaNames())
      next.main.areas[area] = sanitizeNode(next.main.areas[area], seenPanels, seenNodes);
    next.floating = next.floating.filter(floating => {
      if (seenWindows[floating.id])
        return false;
      seenWindows[floating.id] = true;
      floating.root = sanitizeNode(floating.root, seenPanels, seenNodes);
      return floating.root !== null;
    });
    next.hidden = next.hidden.filter(panelId => {
      if (panelById(panelId) === null || seenPanels[panelId])
        return false;
      seenPanels[panelId] = true;
      return true;
    });
    for (const panel of _panels) {
      if (!seenPanels[panel.panelId]) {
        ensurePanelInMain(next, panel.panelId, panel.defaultFrame);
        seenPanels[panel.panelId] = true;
      }
    }
    commit(next);
    return true;
  }

  function sanitizeNode(node: var, seenPanels: var, seenNodes: var): var {
    if (node === null)
      return null;
    if (seenNodes[node.id])
      return null;
    seenNodes[node.id] = true;
    if (node.kind === "frame") {
      node.panels = node.panels.filter(panelId => {
        if (panelById(panelId) === null || seenPanels[panelId])
          return false;
        seenPanels[panelId] = true;
        return true;
      });
      if (node.panels.length === 0)
        return null;
      if (node.panels.indexOf(node.active) < 0)
        node.active = node.panels[0];
      return node;
    }
    const children = [];
    for (const child of node.children) {
      const sanitized = sanitizeNode(child, seenPanels, seenNodes);
      if (sanitized !== null)
        children.push(sanitized);
    }
    node.children = children;
    if (node.children.length === 0)
      return null;
    if (node.children.length === 1)
      return node.children[0];
    node.sizes = Layout.normalizeSizes(node.sizes, node.children.length);
    return node;
  }

  function saveState(): string {
    return JSON.stringify(layout);
  }

  function setSplitSizes(splitId: string, sizes: var): void {
    const next = Layout.clone(layout);
    const split = findNode(next, splitId);
    if (split === null || split.kind !== "split")
      return;
    split.sizes = Layout.normalizeSizes(sizes, split.children.length);
    commit(next);
  }

  function findNode(node: var, nodeId: string): var {
    if (node === null)
      return null;
    const found = Layout.findNode(node, nodeId);
    if (found !== null)
      return found;
    for (const area of Layout.areaNames()) {
      const areaNode = Layout.findNode(node.main.areas[area], nodeId);
      if (areaNode !== null)
        return areaNode;
    }
    for (const floating of node.floating) {
      const floatingNode = Layout.findNode(floating.root, nodeId);
      if (floatingNode !== null)
        return floatingNode;
    }
    return null;
  }

  function setAreaSize(area: string, size: real): void {
    if (Layout.areaNames().indexOf(area) < 0 || size <= 0 || layout.main.sizes[area] === size)
      return;
    const next = Layout.clone(layout);
    next.main.sizes[area] = size;
    commit(next);
  }

  function shutdown(): void {
    _shuttingDown = true;
    for (const windowId of Object.keys(_windows)) {
      _windows[windowId].destroy();
      delete _windows[windowId];
    }
    for (const panel of _panels)
      panel.panelHost = null;
  }

  function syncPanelHosts(): void {
    for (const panel of _panels) {
      const location = Layout.findPanel(layout, panel.panelId);
      let host = null;
      if (location !== null) {
        const frame = Layout.findFrameInLayout(layout, location.frameId);
        if (frame !== null && frame.active === panel.panelId && _frameHosts[location.frameId] !== undefined)
          host = _frameHosts[location.frameId];
      }
      panel.panelHost = host;
    }
  }

  function syncWindows(): void {
    if (_shuttingDown || floatingWindowComponent === null)
      return;
    const wanted = ({});
    for (const floating of layout.floating) {
      wanted[floating.id] = true;
      if (_windows[floating.id] === undefined) {
        const window = floatingWindowComponent.createObject(null, { "controller": root, "windowId": floating.id });
        if (window === null) {
          console.warn("Could not create floating dock window", floating.id);
          continue;
        }
        _windows[floating.id] = window;
        floatingWindowCreated(window);
      }
    }
    for (const windowId of Object.keys(_windows)) {
      if (!wanted[windowId]) {
        _windows[windowId].destroy();
        delete _windows[windowId];
      }
    }
  }

  function uniqueNodeId(prefix: string): string {
    let candidate;
    do {
      candidate = prefix + "-" + _nextId++;
    } while (findNode(layout, candidate) !== null);
    return candidate;
  }

  function uniqueWindowId(): string {
    let candidate;
    do {
      candidate = "floating-" + _nextId++;
    } while (layout.floating.some(floating => floating.id === candidate));
    return candidate;
  }

  function unregisterFrameHost(frameId: string, host: Item): void {
    if (_frameHosts[frameId] === host) {
      delete _frameHosts[frameId];
      syncPanelHosts();
    }
  }

  function unregisterPanel(panel: DockPanel): void {
    panel.dockingController = null;
    _panels = _panels.filter(item => item !== panel);
    panelIds = _panels.map(item => item.panelId);
    const next = Layout.clone(layout);
    Layout.removePanel(next, panel.panelId);
    next.hidden = next.hidden.filter(panelId => panelId !== panel.panelId);
    commit(next);
    panelRegistryChanged();
  }

  function updateFloatingGeometry(windowId: string, x: int, y: int, width: int, height: int): void {
    const floating = layout.floating.find(entry => entry.id === windowId);
    if (floating === undefined)
      return;
    if (floating.x === x && floating.y === y && floating.width === width && floating.height === height)
      return;
    floating.x = x;
    floating.y = y;
    floating.width = Math.max(200, width);
    floating.height = Math.max(140, height);
  }

  function hasOnlyProperties(value: var, allowed: var): bool {
    return Object.keys(value).every(key => allowed.indexOf(key) >= 0);
  }

  function isFiniteNumber(value: var): bool {
    return typeof value === "number" && isFinite(value);
  }

  function isNonEmptyString(value: var): bool {
    return typeof value === "string" && value.length > 0;
  }

  function isPositiveFiniteNumber(value: var): bool {
    return isFiniteNumber(value) && value > 0;
  }

  function isRecord(value: var): bool {
    return value !== null && typeof value === "object" && !Array.isArray(value);
  }

  function isValidLayout(candidate: var): bool {
    if (!isRecord(candidate) || !hasOnlyProperties(candidate, ["version", "main", "floating", "hidden"]) || candidate.version !== 1 || !isRecord(candidate.main) || !hasOnlyProperties(candidate.main, ["areas", "sizes"]) || !isRecord(candidate.main.areas) || !hasOnlyProperties(candidate.main.areas, Layout.areaNames()) || !isRecord(candidate.main.sizes) || !hasOnlyProperties(candidate.main.sizes, ["start", "end", "top", "bottom"]) || !Array.isArray(candidate.floating) || !Array.isArray(candidate.hidden))
      return false;
    for (const area of Layout.areaNames()) {
      if (!validNode(candidate.main.areas[area], true))
        return false;
      if (area !== "center" && !isPositiveFiniteNumber(candidate.main.sizes[area]))
        return false;
    }
    for (const floating of candidate.floating) {
      if (!isRecord(floating) || !hasOnlyProperties(floating, ["id", "root", "returnFrames", "x", "y", "width", "height"]) || !isNonEmptyString(floating.id) || !validNode(floating.root, false) || !isFiniteNumber(floating.x) || !isFiniteNumber(floating.y) || !isFiniteNumber(floating.width) || !isFiniteNumber(floating.height) || floating.width < 200 || floating.height < 140)
        return false;
      if (floating.returnFrames !== undefined) {
        if (!isRecord(floating.returnFrames))
          return false;
        for (const panelId of Object.keys(floating.returnFrames)) {
          if (panelId.length === 0 || !isNonEmptyString(floating.returnFrames[panelId]))
            return false;
        }
      }
    }
    return candidate.hidden.every(panelId => isNonEmptyString(panelId));
  }

  function validNode(node: var, allowNull: bool): bool {
    if (node === null)
      return allowNull;
    if (!isRecord(node) || !isNonEmptyString(node.id))
      return false;
    if (node.kind === "frame") {
      if (!hasOnlyProperties(node, ["id", "kind", "panels", "active", "showLabels", "tabsAtBottom", "headerVisible"]) || !Array.isArray(node.panels) || node.panels.length === 0 || !isNonEmptyString(node.active))
        return false;
      for (const option of ["showLabels", "tabsAtBottom", "headerVisible"]) {
        if (node[option] !== undefined && typeof node[option] !== "boolean")
          return false;
      }
      return node.panels.every(panelId => isNonEmptyString(panelId));
    }
    if (node.kind !== "split" || !hasOnlyProperties(node, ["id", "kind", "orientation", "children", "sizes"]) || (node.orientation !== "horizontal" && node.orientation !== "vertical") || !Array.isArray(node.children) || node.children.length !== 2 || !Array.isArray(node.sizes) || node.sizes.length !== 2)
      return false;
    if (!node.sizes.every(size => isPositiveFiniteNumber(size) && size <= 1))
      return false;
    return node.children.every(child => validNode(child, false));
  }
}
