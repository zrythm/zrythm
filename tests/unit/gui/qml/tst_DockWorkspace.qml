// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtTest
import ZrythmDocking

TestCase {
  id: test

  name: "DockWorkspace"

  property ApplicationWindow window: null

  Component {
    id: windowComponent

    ApplicationWindow {
      id: window

      property alias alphaPanel: alphaPanel
      property alias betaPanel: betaPanel
      property var detachedWindow: null
      property string initialLayout: ""
      property alias workspace: workspace

      height: 600
      visible: true
      width: 900

      DockWorkspace {
        id: workspace

        anchors.fill: parent
        defaultLayout: test.defaultLayout()
        dockPalette: window.palette
        initialLayout: window.initialLayout
        workspaceId: "workspace-visual-test"

        DockPanel {
          id: alphaPanel

          property int preservedState: 0

          defaultFrame: "left"
          panelId: "alpha"
          title: "Alpha"

          Rectangle {
            anchors.fill: parent
            color: window.palette.base

            Label {
              anchors.centerIn: parent
              text: "Alpha mock content"
            }
          }
        }

        DockPanel {
          id: betaPanel

          defaultFrame: "left"
          panelId: "beta"
          title: "Beta"

          Label {
            anchors.centerIn: parent
            text: "Beta mock content"
          }
        }

        DockPanel {
          id: gammaPanel

          defaultFrame: "right"
          panelId: "gamma"
          title: "Gamma"

          Label {
            anchors.centerIn: parent
            text: "Gamma mock content"
          }
        }

        DockPanel {
          id: centerPanel

          closable: false
          defaultFrame: "center"
          floatable: false
          movable: false
          panelId: "center"
          title: "Center"

          Label {
            anchors.centerIn: parent
            text: "Fixed center mock content"
          }
        }
      }

      Connections {
        function onFloatingWindowCreated(floatingWindow: var) {
          window.detachedWindow = floatingWindow;
        }

        target: workspace.dockingController
      }
    }
  }

  function cleanup() {
    if (window !== null)
      window.destroy();
    window = null;
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

  function init() {
    window = windowComponent.createObject(null);
    tryVerify(() => window.workspace.dockingController.panelById("alpha") !== null);
  }

  function test_allDeclaredPanelsAreRegistered() {
    const controller = window.workspace.dockingController;
    compare(controller.panelIds.slice().sort().join(","), "alpha,beta,center,gamma");
    for (const panelId of ["alpha", "beta", "center", "gamma"])
      verify(controller.panelById(panelId) !== null, panelId);
  }

  function test_initialLayoutRestoresAfterPanelRegistration() {
    window.destroy();
    const restoredLayout = defaultLayout();
    restoredLayout.main.areas.start.active = "beta";
    window = windowComponent.createObject(null, {
      "initialLayout": JSON.stringify(restoredLayout)
    });
    tryVerify(() => window.workspace.dockingController.panelIds.length === 4);
    compare(window.workspace.dockingController.layout.main.areas.start.active, "beta");
    tryVerify(() => window.betaPanel.panelHost !== null);
    compare(window.alphaPanel.panelHost, null);
  }

  function test_activePanelIsHostedAndInactivePanelIsStored() {
    tryVerify(() => window.alphaPanel.panelHost !== null);
    verify(window.alphaPanel.visible);
    compare(window.betaPanel.panelHost, null);
    verify(!window.betaPanel.visible);
  }

  function test_detachedWindowUsesActivePanelTitle() {
    const controller = window.workspace.dockingController;
    verify(controller.detachPanel("alpha"));
    tryVerify(() => window.detachedWindow !== null);
    compare(window.detachedWindow.title, "Alpha");
    verify(controller.movePanel("beta", controller.panelLocation("alpha").frameId, "tab", 1));
    compare(window.detachedWindow.title, "Beta");
  }

  function test_restoreRepositionsExistingFloatingWindow() {
    const controller = window.workspace.dockingController;
    verify(controller.detachPanel("alpha"));
    tryVerify(() => window.detachedWindow !== null);
    const windowId = controller.panelLocation("alpha").windowId;
    const savedX = window.detachedWindow.x + 120;

    window.detachedWindow.x = savedX;
    controller.updateFloatingGeometry(
      windowId, window.detachedWindow.x, window.detachedWindow.y,
      window.detachedWindow.width, window.detachedWindow.height);
    const savedLayout = controller.saveState();

    window.detachedWindow.x = savedX + 80;
    controller.updateFloatingGeometry(
      windowId, window.detachedWindow.x, window.detachedWindow.y,
      window.detachedWindow.width, window.detachedWindow.height);

    verify(controller.restoreState(savedLayout));
    compare(window.detachedWindow.x, savedX);
    controller.updateFloatingGeometry(
      windowId, window.detachedWindow.x, window.detachedWindow.y,
      window.detachedWindow.width, window.detachedWindow.height);
  }

  function test_detachAndReattachPreserveContentObject() {
    const controller = window.workspace.dockingController;
    const originalPanel = window.alphaPanel;
    window.alphaPanel.preservedState = 42;
    verify(controller.detachPanel("alpha"));
    tryCompare(controller, "floatingWindowCount", 1);
    tryVerify(() => window.alphaPanel.panelHost !== null);
    compare(controller.panelById("alpha"), originalPanel);
    compare(window.alphaPanel.preservedState, 42);
    const windowId = controller.panelLocation("alpha").windowId;
    verify(controller.closeFloating(windowId));
    tryCompare(controller, "floatingWindowCount", 0);
    tryVerify(() => window.alphaPanel.panelHost !== null);
    compare(controller.panelById("alpha"), originalPanel);
    compare(window.alphaPanel.preservedState, 42);
  }

  function test_splitPreservesContentObject() {
    const controller = window.workspace.dockingController;
    const originalPanel = window.alphaPanel;
    window.alphaPanel.preservedState = 17;
    verify(controller.movePanel("alpha", "right", "left", 0));
    tryVerify(() => window.alphaPanel.panelHost !== null);
    compare(controller.panelById("alpha"), originalPanel);
    compare(window.alphaPanel.preservedState, 17);
  }

  function test_restoreRecreatesFloatingWindowAndContent() {
    const controller = window.workspace.dockingController;
    const originalPanel = window.alphaPanel;
    verify(controller.detachPanel("alpha"));
    const saved = controller.saveState();
    controller.resetLayout();
    tryCompare(controller, "floatingWindowCount", 0);
    verify(controller.restoreState(saved));
    tryCompare(controller, "floatingWindowCount", 1);
    tryVerify(() => window.alphaPanel.panelHost !== null);
    compare(controller.panelById("alpha"), originalPanel);
  }
}
