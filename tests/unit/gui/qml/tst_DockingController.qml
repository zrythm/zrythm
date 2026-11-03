// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtTest
import ZrythmDocking

TestCase {
  id: test

  name: "DockingController"

  property DockingController controller: null
  property var panels: []
  property Item storage: null
  readonly property Palette testPalette: Palette {
    alternateBase: "#202020"
    highlight: "#40a0ff"
  }

  Component {
    id: controllerComponent

    DockingController {
      dockPalette: test.testPalette
    }
  }

  Component {
    id: panelComponent

    DockPanel {
      storageHost: test.storage
    }
  }

  Component {
    id: storageComponent

    Item {
    }
  }

  function cleanup() {
    if (controller !== null)
      controller.shutdown();
    for (const panel of panels)
      panel.destroy();
    panels = [];
    if (controller !== null)
      controller.destroy();
    controller = null;
    if (storage !== null)
      storage.destroy();
    storage = null;
  }

  function defaultLayout(): var {
    return {
      "version": 1,
      "main": {
        "areas": {
          "start": { "id": "left", "kind": "frame", "panels": ["alpha", "beta"], "active": "alpha" },
          "end": { "id": "right", "kind": "frame", "panels": ["gamma"], "active": "gamma" },
          "top": null,
          "bottom": null,
          "center": { "id": "center", "kind": "frame", "panels": ["center"], "active": "center", "headerVisible": false }
        },
        "sizes": { "start": 220, "end": 240, "top": 180, "bottom": 200 }
      },
      "floating": [],
      "hidden": []
    };
  }

  function splitLayout(): var {
    const candidate = defaultLayout();
    candidate.main.areas.start.panels = ["beta"];
    candidate.main.areas.start.active = "beta";
    candidate.main.areas.center = {
      "id": "center-split",
      "kind": "split",
      "orientation": "horizontal",
      "children": [
        { "id": "center", "kind": "frame", "panels": ["center"], "active": "center" },
        { "id": "center-alpha", "kind": "frame", "panels": ["alpha"], "active": "alpha" }
      ],
      "sizes": [0.75, 0.25]
    };
    return candidate;
  }

  function floatingLayout(): var {
    const candidate = defaultLayout();
    candidate.main.areas.start.panels = ["alpha"];
    candidate.main.areas.start.active = "alpha";
    candidate.floating = [{
      "id": "floating-1",
      "root": { "id": "floating-frame", "kind": "frame", "panels": ["beta"], "active": "beta" },
      "returnFrames": { "beta": "left" },
      "x": 80,
      "y": -40,
      "width": 480,
      "height": 360
    }];
    return candidate;
  }

  function init() {
    storage = storageComponent.createObject(null);
    controller = controllerComponent.createObject(null);
    controller.defaultLayout = defaultLayout();
    controller.resetLayout();
    panels = [];
    for (const panelId of ["alpha", "beta", "gamma", "center"]) {
      const defaultFrame = panelId === "gamma" ? "right" : (panelId === "center" ? "center" : "left");
      const panel = panelComponent.createObject(null, {
        "panelId": panelId,
        "title": panelId,
        "defaultFrame": defaultFrame,
        "movable": panelId !== "center",
        "floatable": panelId !== "center",
        "closable": panelId !== "center"
      });
      panels.push(panel);
      verify(controller.registerPanel(panel));
    }
  }

  function panelOccurrences(panelId: string): int {
    let count = 0;
    for (const area of ["start", "end", "top", "bottom", "center"])
      count += occurrencesInNode(controller.layout.main.areas[area], panelId);
    for (const floating of controller.layout.floating)
      count += occurrencesInNode(floating.root, panelId);
    return count;
  }

  function occurrencesInNode(node: var, panelId: string): int {
    if (node === null)
      return 0;
    if (node.kind === "frame")
      return node.panels.filter(id => id === panelId).length;
    let count = 0;
    for (const child of node.children)
      count += occurrencesInNode(child, panelId);
    return count;
  }

  function verifyNoEmptyFramesInNode(node: var): void {
    if (node === null)
      return;
    if (node.kind === "frame") {
      verify(node.panels.length > 0);
      return;
    }
    for (const child of node.children)
      verifyNoEmptyFramesInNode(child);
  }

  function verifyLayoutHasNoEmptyFrames(): void {
    for (const area of ["start", "end", "top", "bottom", "center"])
      verifyNoEmptyFramesInNode(controller.layout.main.areas[area]);
    for (const floating of controller.layout.floating)
      verifyNoEmptyFramesInNode(floating.root);
  }

  function test_defaultLayoutContainsEveryPanelOnce() {
    for (const panelId of controller.panelIds)
      compare(panelOccurrences(panelId), 1, panelId);
    compare(controller.layout.main.areas.start.active, "alpha");
  }

  function test_moveToAnotherFrameActivatesPanel() {
    verify(controller.movePanel("alpha", "right", "tab", 1));
    compare(controller.panelLocation("alpha").frameId, "right");
    compare(controller.layout.main.areas.end.panels.join(","), "gamma,alpha");
    compare(controller.layout.main.areas.end.active, "alpha");
    compare(controller.layout.main.areas.start.panels.join(","), "beta");
    compare(panelOccurrences("alpha"), 1);
  }

  function test_reorderWithinFrame() {
    verify(controller.movePanel("alpha", "left", "tab", 2));
    compare(controller.layout.main.areas.start.panels.join(","), "beta,alpha");
    compare(controller.layout.main.areas.start.active, "alpha");
  }

  function test_edgeDropCreatesNestedSplit() {
    verify(controller.movePanel("alpha", "right", "left", 0));
    const root = controller.layout.main.areas.end;
    compare(root.kind, "split");
    compare(root.orientation, "horizontal");
    compare(root.children.length, 2);
    compare(root.children[0].panels[0], "alpha");
    compare(root.children[1].panels[0], "gamma");
    compare(panelOccurrences("alpha"), 1);
  }

  function test_singlePanelFrameOnlyAcceptsTabDrop() {
    const saved = controller.saveState();
    verify(controller.canMovePanel("gamma", "right", "tab"));
    for (const zone of ["left", "right", "top", "bottom"]) {
      verify(!controller.canMovePanel("gamma", "right", zone));
      verify(!controller.movePanel("gamma", "right", zone, 0));
      compare(controller.saveState(), saved);
    }
    verifyLayoutHasNoEmptyFrames();
  }

  function test_movePanelToEmptyAreaAndBackLeavesNoEmptyFrames() {
    verify(controller.canMovePanelToArea("gamma", "top"));
    verify(controller.movePanelToArea("gamma", "top"));
    compare(controller.panelLocation("gamma").area, "top");
    compare(controller.layout.main.areas.end, null);
    verifyLayoutHasNoEmptyFrames();

    verify(controller.canMovePanelToArea("gamma", "end"));
    verify(controller.movePanelToArea("gamma", "end"));
    compare(controller.panelLocation("gamma").area, "end");
    compare(controller.layout.main.areas.top, null);
    verifyLayoutHasNoEmptyFrames();
  }

  function test_detachAndCloseRestoresOriginalFrame() {
    const originalPanel = controller.panelById("alpha");
    verify(controller.canDetachPanel("alpha"));
    verify(!controller.canDetachPanel("center"));
    verify(controller.detachPanel("alpha"));
    verify(!controller.canDetachPanel("alpha"));
    compare(controller.floatingWindowCount, 1);
    const floatingLocation = controller.panelLocation("alpha");
    verify(floatingLocation.windowId !== null);
    verify(controller.closeFloating(floatingLocation.windowId));
    compare(controller.floatingWindowCount, 0);
    compare(controller.panelLocation("alpha").frameId, "left");
    compare(controller.panelById("alpha"), originalPanel);
    compare(panelOccurrences("alpha"), 1);
  }

  function test_fixedPanelCannotMoveOrDetach() {
    verify(!controller.movePanel("center", "right", "tab", 0));
    verify(!controller.detachPanel("center"));
  }

  function test_floatingGeometryUpdateKeepsLayoutObject() {
    verify(controller.detachPanel("beta"));
    const windowId = controller.panelLocation("beta").windowId;
    const layoutBeforeUpdate = controller.layout;

    controller.updateFloatingGeometry(windowId, 320, 240, 520, 380);

    verify(controller.layout === layoutBeforeUpdate);
    compare(controller.layout.floating[0].x, 320);
    compare(controller.layout.floating[0].y, 240);
    compare(controller.layout.floating[0].width, 520);
    compare(controller.layout.floating[0].height, 380);
    const saved = JSON.parse(controller.saveState());
    compare(saved.floating[0].x, 320);
    compare(saved.floating[0].y, 240);
    compare(saved.floating[0].width, 520);
    compare(saved.floating[0].height, 380);
  }

  function test_hideAndPresentPanel() {
    verify(controller.hidePanel("beta"));
    verify(controller.isPanelHidden("beta"));
    compare(controller.panelLocation("beta"), null);
    verify(controller.presentPanel("beta"));
    verify(!controller.isPanelHidden("beta"));
    compare(controller.panelLocation("beta").frameId, "left");
    compare(controller.layout.main.areas.start.active, "beta");
  }

  function test_stateRoundTripPreservesSplitAndFloatingLayout() {
    verify(controller.movePanel("alpha", "right", "top", 0));
    verify(controller.detachPanel("beta"));
    const saved = controller.saveState();
    controller.resetLayout();
    compare(controller.floatingWindowCount, 0);
    verify(controller.restoreState(saved));
    compare(controller.saveState(), saved);
    compare(controller.floatingWindowCount, 1);
    compare(controller.layout.main.areas.end.kind, "split");
  }

  function test_malformedStateKeepsCurrentLayout() {
    const saved = controller.saveState();
    verify(!controller.restoreState("not json"));
    compare(controller.saveState(), saved);
    verify(!controller.restoreState("{\"version\":99}"));
    compare(controller.saveState(), saved);
  }

  function test_validSplitAndFloatingStatesRestore() {
    verify(controller.restoreState(JSON.stringify(splitLayout())));
    compare(controller.layout.main.areas.start.active, "beta");
    compare(controller.layout.main.areas.center.kind, "split");
    compare(controller.layout.main.areas.center.sizes.join(","), "0.75,0.25");
    for (const panelId of controller.panelIds)
      compare(panelOccurrences(panelId), 1, panelId);

    verify(controller.restoreState(JSON.stringify(floatingLayout())));
    compare(controller.layout.floating.length, 1);
    compare(controller.layout.floating[0].x, 80);
    compare(controller.layout.floating[0].y, -40);
    compare(controller.layout.floating[0].width, 480);
    compare(controller.layout.floating[0].height, 360);
    compare(controller.layout.floating[0].returnFrames.beta, "left");
    compare(controller.panelLocation("beta").windowId, "floating-1");
  }

  function test_invalidStructuralStatesKeepCurrentLayout_data() {
    const nullMain = defaultLayout();
    nullMain.main = null;
    const nullAreas = defaultLayout();
    nullAreas.main.areas = null;
    const missingArea = defaultLayout();
    delete missingArea.main.areas.top;
    const nullSplitChild = splitLayout();
    nullSplitChild.main.areas.center.children[0] = null;
    const singleSplitChild = splitLayout();
    singleSplitChild.main.areas.center.children.splice(1, 1);
    singleSplitChild.main.areas.center.sizes = [1];
    const emptyFrame = defaultLayout();
    emptyFrame.main.areas.start.panels = [];
    const invalidFrameOption = defaultLayout();
    invalidFrameOption.main.areas.start.showLabels = "false";
    const unknownProperty = defaultLayout();
    unknownProperty.unexpected = true;
    const nullFloating = floatingLayout();
    nullFloating.floating = [null];
    const invalidReturnFrames = floatingLayout();
    invalidReturnFrames.floating[0].returnFrames.beta = 42;
    return [
      { "tag": "root array", "serialized": "[]" },
      { "tag": "null main", "serialized": JSON.stringify(nullMain) },
      { "tag": "null areas", "serialized": JSON.stringify(nullAreas) },
      { "tag": "missing area", "serialized": JSON.stringify(missingArea) },
      { "tag": "null split child", "serialized": JSON.stringify(nullSplitChild) },
      { "tag": "single split child", "serialized": JSON.stringify(singleSplitChild) },
      { "tag": "empty frame", "serialized": JSON.stringify(emptyFrame) },
      { "tag": "non-boolean frame option", "serialized": JSON.stringify(invalidFrameOption) },
      { "tag": "unknown property", "serialized": JSON.stringify(unknownProperty) },
      { "tag": "null floating entry", "serialized": JSON.stringify(nullFloating) },
      { "tag": "invalid return frame", "serialized": JSON.stringify(invalidReturnFrames) }
    ];
  }

  function test_invalidStructuralStatesKeepCurrentLayout(row) {
    const saved = controller.saveState();
    verify(!controller.restoreState(row.serialized), row.tag);
    compare(controller.saveState(), saved, row.tag);
  }

  function test_invalidSplitSizesKeepCurrentLayout_data() {
    const missingSizes = splitLayout();
    delete missingSizes.main.areas.center.sizes;
    const wrongCount = splitLayout();
    wrongCount.main.areas.center.sizes = [1];
    const zero = splitLayout();
    zero.main.areas.center.sizes = [0.5, 0];
    const negative = splitLayout();
    negative.main.areas.center.sizes = [1.5, -0.5];
    const nonNumeric = splitLayout();
    nonNumeric.main.areas.center.sizes = [0.5, "0.5"];
    const tooLarge = splitLayout();
    tooLarge.main.areas.center.sizes = [0.5, 1.5];
    const overflow = JSON.stringify(splitLayout()).replace("\"sizes\":[0.75,0.25]", "\"sizes\":[0.75,1e400]");
    return [
      { "tag": "missing sizes", "serialized": JSON.stringify(missingSizes) },
      { "tag": "size count", "serialized": JSON.stringify(wrongCount) },
      { "tag": "zero size", "serialized": JSON.stringify(zero) },
      { "tag": "negative size", "serialized": JSON.stringify(negative) },
      { "tag": "non-numeric size", "serialized": JSON.stringify(nonNumeric) },
      { "tag": "unnormalized size", "serialized": JSON.stringify(tooLarge) },
      { "tag": "non-finite size", "serialized": overflow }
    ];
  }

  function test_invalidSplitSizesKeepCurrentLayout(row) {
    const saved = controller.saveState();
    verify(!controller.restoreState(row.serialized), row.tag);
    compare(controller.saveState(), saved, row.tag);
  }

  function test_invalidFloatingGeometryKeepsCurrentLayout_data() {
    const nonNumericX = floatingLayout();
    nonNumericX.floating[0].x = "80";
    const narrow = floatingLayout();
    narrow.floating[0].width = 199;
    const short = floatingLayout();
    short.floating[0].height = 139;
    const infiniteX = JSON.stringify(floatingLayout()).replace("\"x\":80", "\"x\":1e400");
    const infiniteY = JSON.stringify(floatingLayout()).replace("\"y\":-40", "\"y\":1e400");
    const infiniteWidth = JSON.stringify(floatingLayout()).replace("\"width\":480", "\"width\":1e400");
    return [
      { "tag": "non-numeric x", "serialized": JSON.stringify(nonNumericX) },
      { "tag": "narrow window", "serialized": JSON.stringify(narrow) },
      { "tag": "short window", "serialized": JSON.stringify(short) },
      { "tag": "non-finite x", "serialized": infiniteX },
      { "tag": "non-finite y", "serialized": infiniteY },
      { "tag": "non-finite width", "serialized": infiniteWidth }
    ];
  }

  function test_invalidFloatingGeometryKeepsCurrentLayout(row) {
    const saved = controller.saveState();
    verify(!controller.restoreState(row.serialized), row.tag);
    compare(controller.saveState(), saved, row.tag);
  }

  function test_restoreSanitizesDuplicateAndUnknownPanelIds() {
    const candidate = defaultLayout();
    candidate.main.areas.start.panels = ["alpha", "alpha", "missing", "beta", "beta"];
    candidate.main.areas.start.active = "missing";
    candidate.hidden = ["alpha", "alpha", "missing"];
    verify(controller.restoreState(JSON.stringify(candidate)));
    compare(controller.layout.main.areas.start.panels.join(","), "alpha,beta");
    compare(controller.layout.main.areas.start.active, "alpha");
    compare(controller.layout.hidden.length, 0);
    for (const panelId of controller.panelIds)
      compare(panelOccurrences(panelId), 1, panelId);
  }

  function test_restoreSanitizesDuplicateNodeIds() {
    const candidate = defaultLayout();
    candidate.main.areas.top = {
      "id": "left",
      "kind": "frame",
      "panels": ["beta"],
      "active": "beta"
    };
    verify(controller.restoreState(JSON.stringify(candidate)));
    compare(controller.layout.main.areas.top, null);
    compare(controller.panelLocation("beta").frameId, "left");
  }

  function test_restoreSanitizesDuplicateWindowIds() {
    const candidate = floatingLayout();
    candidate.main.areas.end = null;
    candidate.floating.push({
      "id": "floating-1",
      "root": { "id": "second-floating-frame", "kind": "frame", "panels": ["gamma"], "active": "gamma" },
      "x": 100,
      "y": 100,
      "width": 480,
      "height": 360
    });
    verify(controller.restoreState(JSON.stringify(candidate)));
    compare(controller.layout.floating.length, 1);
    compare(controller.panelLocation("beta").windowId, "floating-1");
    compare(controller.panelLocation("gamma").area, "center");
  }
}
