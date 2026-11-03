// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

.pragma library

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function emptyLayout() {
  return {
    "version": 1,
    "main": {
      "areas": { "start": null, "end": null, "top": null, "bottom": null, "center": null },
      "sizes": { "start": 240, "end": 240, "top": 200, "bottom": 240 }
    },
    "floating": [],
    "hidden": []
  };
}

function makeFrame(frameId, panelIds, activePanelId) {
  return {
    "id": frameId,
    "kind": "frame",
    "panels": panelIds.slice(),
    "active": activePanelId
  };
}

function areaNames() {
  return ["start", "end", "top", "bottom", "center"];
}

function findNode(node, nodeId) {
  if (node === null || node === undefined)
    return null;
  if (node.id === nodeId)
    return node;
  if (node.kind === "split") {
    for (const child of node.children) {
      const found = findNode(child, nodeId);
      if (found !== null)
        return found;
    }
  }
  return null;
}

function findFrameInLayout(layout, frameId) {
  for (const area of areaNames()) {
    const found = findNode(layout.main.areas[area], frameId);
    if (found !== null && found.kind === "frame")
      return found;
  }
  for (const floating of layout.floating) {
    const found = findNode(floating.root, frameId);
    if (found !== null && found.kind === "frame")
      return found;
  }
  return null;
}

function findPanelInNode(node, panelId, area, windowId) {
  if (node === null || node === undefined)
    return null;
  if (node.kind === "frame" && node.panels.indexOf(panelId) >= 0) {
    return { "frameId": node.id, "area": area, "windowId": windowId };
  }
  if (node.kind === "split") {
    for (const child of node.children) {
      const found = findPanelInNode(child, panelId, area, windowId);
      if (found !== null)
        return found;
    }
  }
  return null;
}

function findPanel(layout, panelId) {
  for (const area of areaNames()) {
    const found = findPanelInNode(layout.main.areas[area], panelId, area, null);
    if (found !== null)
      return found;
  }
  for (const floating of layout.floating) {
    const found = findPanelInNode(floating.root, panelId, null, floating.id);
    if (found !== null)
      return found;
  }
  return null;
}

function normalizeSizes(sizes, count) {
  let result = Array.isArray(sizes) ? sizes.slice(0, count) : [];
  while (result.length < count)
    result.push(1);
  let total = 0;
  for (let i = 0; i < count; ++i) {
    if (typeof result[i] !== "number" || !isFinite(result[i]) || result[i] <= 0)
      result[i] = 1;
    total += result[i];
  }
  if (total <= 0)
    return result.map(() => 1 / count);
  return result.map(size => size / total);
}

function normalizeNode(node) {
  if (node === null || node === undefined)
    return null;
  if (node.kind === "frame") {
    if (node.panels.length === 0)
      return null;
    if (node.panels.indexOf(node.active) < 0)
      node.active = node.panels[0];
    return node;
  }
  if (node.kind !== "split")
    return null;

  const normalizedChildren = [];
  for (const child of node.children) {
    const normalized = normalizeNode(child);
    if (normalized !== null)
      normalizedChildren.push(normalized);
  }
  node.children = normalizedChildren;
  if (node.children.length === 0)
    return null;
  if (node.children.length === 1)
    return node.children[0];
  node.sizes = normalizeSizes(node.sizes, node.children.length);
  return node;
}

function normalizeLayout(layout) {
  for (const area of areaNames())
    layout.main.areas[area] = normalizeNode(layout.main.areas[area]);
  layout.floating = layout.floating.filter(floating => {
    floating.root = normalizeNode(floating.root);
    return floating.root !== null;
  });
  return layout;
}

function removePanel(layout, panelId) {
  for (const area of areaNames()) {
    const root = layout.main.areas[area];
    if (root === null)
      continue;
    if (root.kind === "frame")
      root.panels = root.panels.filter(id => id !== panelId);
    else
      removePanelFromNode(root, panelId);
  }
  for (const floating of layout.floating) {
    if (floating.root.kind === "frame")
      floating.root.panels = floating.root.panels.filter(id => id !== panelId);
    else
      removePanelFromNode(floating.root, panelId);
  }
  return normalizeLayout(layout);
}

function removePanelFromNode(node, panelId) {
  if (node.kind === "frame") {
    node.panels = node.panels.filter(id => id !== panelId);
    return;
  }
  for (const child of node.children)
    removePanelFromNode(child, panelId);
}

function insertIntoFrame(layout, frameId, panelId, tabIndex) {
  const frame = findFrameInLayout(layout, frameId);
  if (frame === null)
    return false;
  let index = tabIndex === undefined || tabIndex === null ? frame.panels.length : tabIndex;
  index = Math.max(0, Math.min(frame.panels.length, index));
  frame.panels.splice(index, 0, panelId);
  frame.active = panelId;
  return true;
}

function firstMainFrame(layout) {
  for (const area of ["center", "start", "end", "bottom", "top"]) {
    const root = layout.main.areas[area];
    if (root === null)
      continue;
    if (root.kind === "frame")
      return root;
    const frame = firstFrameInNode(root);
    if (frame !== null)
      return frame;
  }
  return null;
}

function firstFrameInNode(node) {
  if (node.kind === "frame")
    return node;
  for (const child of node.children) {
    const frame = firstFrameInNode(child);
    if (frame !== null)
      return frame;
  }
  return null;
}

function replaceInNode(node, nodeId, replacement) {
  if (node.id === nodeId)
    return replacement;
  if (node.kind === "split") {
    for (let i = 0; i < node.children.length; ++i)
      node.children[i] = replaceInNode(node.children[i], nodeId, replacement);
  }
  return node;
}

function replaceNode(layout, nodeId, replacement) {
  for (const area of areaNames()) {
    if (layout.main.areas[area] !== null)
      layout.main.areas[area] = replaceInNode(layout.main.areas[area], nodeId, replacement);
  }
  for (const floating of layout.floating)
    floating.root = replaceInNode(floating.root, nodeId, replacement);
}

function collectNodeIds(node, result) {
  if (node === null || node === undefined)
    return;
  result.push(node.id);
  if (node.kind === "split") {
    for (const child of node.children)
      collectNodeIds(child, result);
  }
}

function collectPanelIds(node, result) {
  if (node === null || node === undefined)
    return;
  if (node.kind === "frame") {
    for (const panelId of node.panels)
      result.push(panelId);
  } else if (node.kind === "split") {
    for (const child of node.children)
      collectPanelIds(child, result);
  }
}
