// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import Zrythm

TestCase {
  id: testCase

  name: "ModularCanvas"

  // The canvas needs a visible window: pointer events are only delivered
  // to items inside an exposed window.
  Component {
    id: windowComponent

    ApplicationWindow {
      property alias canvas: canvas
      property alias edgeModel: edges

      height: 480
      visible: true
      width: 640

      ModularCanvas {
        id: canvas

        height: 480
        width: 640

        edgeModel: ListModel {
          id: edges

          ListElement {
            automatic: false
            destinationNode: "destination-node"
            destinationPort: "gain"
            edgeId: "project:existing"
            readOnly: false
            signalKind: "parameter"
            sourceNode: "source-node"
            sourcePort: "cv-out"
          }
        }

        nodeModel: ListModel {
          ListElement {
            accentKind: "cv"
            hasModulatableParameters: false
            inputsJson: "[]"
            nodeId: "source-node"
            nodeSubtitle: ""
            nodeTitle: "Source"
            nodeType: "Processor"
            outputsJson: "[{\"id\":\"cv-out\",\"label\":\"CV out\",\"kind\":\"cv\"}]"
            parametersJson: "[]"
            nodeX: 0
            nodeY: 0
          }

          ListElement {
            accentKind: "neutral"
            hasModulatableParameters: true
            inputsJson: "[]"
            nodeId: "destination-node"
            nodeSubtitle: ""
            nodeTitle: "Destination"
            nodeType: "Plugin"
            outputsJson: "[]"
            parametersJson: "[{\"id\":\"gain\",\"label\":\"Gain\",\"kind\":\"parameter\",\"connections\":[{\"sourcePortId\":\"cv-out\",\"multiplier\":0.75,\"locked\":false}]},{\"id\":\"cutoff\",\"label\":\"Cutoff\",\"kind\":\"parameter\"}]"
            nodeX: 320
            nodeY: 0
          }
        }
      }
    }
  }

  SignalSpy {
    id: parameterSpy

    signalName: "parameterConnectionRequested"
    target: null
  }

  SignalSpy {
    id: connectSpy

    signalName: "projectConnectRequested"
    target: null
  }

  property ApplicationWindow window: null

  function init(): void {
    window = windowComponent.createObject(null);
    tryVerify(() => window.visible);
    parameterSpy.target = window.canvas;
    connectSpy.target = window.canvas;
    parameterSpy.clear();
    connectSpy.clear();
    window.canvas.cancelPendingConnection();
    window.canvas.selectedConnectionId = "";
    window.canvas.message = "";
  }

  function cleanup(): void {
    parameterSpy.target = null;
    connectSpy.target = null;
    window.destroy();
    window = null;
  }

  function test_dropOnParameterSlotConnectsDirectly(): void {
    window.canvas.beginConnection("source-node", {
        "flow": "output",
        "id": "cv-out",
        "kind": "cv"
      });
    window.canvas.completeConnection("destination-node", {
        "flow": "input",
        "id": "cutoff",
        "kind": "parameter"
      });

    compare(connectSpy.count, 1);
    compare(connectSpy.signalArguments[0][0], "cv-out");
    compare(connectSpy.signalArguments[0][1], "cutoff");
    compare(parameterSpy.count, 0);
    verify(!window.canvas.hasPendingConnection);
  }

  function test_dropOnParameterMenuOpensSelector(): void {
    window.canvas.beginConnection("source-node", {
        "flow": "output",
        "id": "cv-out",
        "kind": "cv"
      });
    window.canvas.completeConnection("destination-node", {
        "flow": "input",
        "id": "parameter-menu",
        "kind": "parameter",
        "transient": true
      });

    compare(parameterSpy.count, 1);
    compare(parameterSpy.signalArguments[0][0], "destination-node");
    compare(parameterSpy.signalArguments[0][1], "cv-out");
    compare(connectSpy.count, 0);
    verify(!window.canvas.hasPendingConnection);
  }

  function test_dropOnConnectedParameterIsNoOp(): void {
    window.canvas.beginConnection("source-node", {
        "flow": "output",
        "id": "cv-out",
        "kind": "cv"
      });
    window.canvas.completeConnection("destination-node", {
        "flow": "input",
        "id": "gain",
        "kind": "parameter"
      });

    compare(connectSpy.count, 0);
    compare(parameterSpy.count, 0);
    compare(window.canvas.message, "");
    verify(!window.canvas.hasPendingConnection);
    compare(window.edgeModel.count, 1);
  }

  function test_clickOnPortStartsAndCancelsConnection(): void {
    verify(!window.canvas.hasPendingConnection);
    const center = window.canvas.canvasPointFromWorld(
        window.canvas.portCenter("source-node", "cv-out", true));
    mouseClick(window.canvas, center.x, center.y);
    verify(window.canvas.hasPendingConnection);

    // Clicking the pending connection's source port again cancels it.
    mouseClick(window.canvas, center.x, center.y);
    verify(!window.canvas.hasPendingConnection);
    compare(connectSpy.count, 0);
  }
}
