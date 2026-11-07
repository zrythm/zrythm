// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmActions
import ZrythmGui
import ZrythmStyle

FocusScope {
  id: root

  readonly property real defaultZoom: 1.0
  property var edgeModel: null
  // Source and destination port IDs of the connection whose modulation
  // knob is hovered; empty strings mean nothing is highlighted. Matching
  // edges render highlighted.
  property string highlightedSourcePort: ""
  property string highlightedDestinationPort: ""
  readonly property int headerHeight: 40
  property int layoutRevision: 0
  property bool autoFitOnModelChange: true
  property string linkedNodeId: ""
  readonly property real maximumZoom: 2.5
  property string message: ""
  readonly property real minimumZoom: 0.3
  property var nodeModel: null
  readonly property int nodeWidth: 236
  // The project model, passed to the nodes for compact modulator editors.
  property ModularProjectModel modularModel: null
  // Pushes undoable parameter edits, passed to the nodes' editors.
  property ProcessorParameterOperator parameterOperator: null
  property real panX: 0
  property real panY: 0
  property string pendingFlow: ""
  property string pendingNodeId: ""
  property point pendingPoint: Qt.point(0, 0)
  property string pendingPortId: ""
  property string pendingSignalKind: ""
  readonly property int portHeight: 26
  property bool internalDeleteShortcutEnabled: true
  property string selectedConnectionId: ""
  property string selectedNodeId: ""
  readonly property int worldHeight: 1500
  readonly property int worldWidth: 2400
  property real zoom: defaultZoom

  readonly property bool hasPendingConnection: pendingNodeId !== ""
  readonly property bool hasDeletableSelection: selectedConnectionId !== ""
    && edgeIsDeletable(selectedConnectionId)

  signal nodeMoveFinished(string nodeId, real x, real y)
  signal nodeMenuRequested(string nodeId)
  signal nodeSelected(string nodeId)
  signal parameterConnectionRequested(string nodeId, string sourcePortId)
  signal parameterEditActiveChanged(bool active)
  signal parameterMultiplierEdited(string sourcePortId, string destinationPortId, real value)
  signal projectConnectRequested(string sourcePortId, string destinationPortId)
  signal projectDisconnectRequested(string sourcePortId, string destinationPortId)

  property string autoScrollNodeId: ""
  property point autoScrollPointer: Qt.point(0, 0)
  property bool autoScrollPointerValid: false

  function canvasPointFromWorld(worldPoint: point): point {
    return Qt.point(worldPoint.x * zoom + panX, worldPoint.y * zoom + panY);
  }

  function nodeItemFor(nodeId: string): ModularNode {
    for (let i = 0; i < nodeRepeater.count; ++i) {
      const item = nodeRepeater.itemAt(i) as ModularNode;
      if (item !== null && item.nodeId === nodeId)
        return item;
    }
    return null;
  }

  function beginAutoScroll(nodeId: string) {
    autoScrollNodeId = nodeId;
    autoScrollPointerValid = false;
  }

  function updateAutoScroll(worldPointer: point) {
    autoScrollPointer = canvasPointFromWorld(worldPointer);
    autoScrollPointerValid = true;
    if (!autoScrollTimer.running)
      autoScrollTimer.start();
  }

  function endAutoScroll() {
    autoScrollTimer.stop();
    autoScrollNodeId = "";
    autoScrollPointerValid = false;
  }

  function autoScrollTick() {
    if (autoScrollNodeId === "" || !autoScrollPointerValid || zoom <= 0)
      return;
    const marginX = Math.min(24, width / 8);
    const marginY = Math.min(24, height / 8);
    const maxSpeed = 22;
    let fractionX = 0;
    let fractionY = 0;
    if (autoScrollPointer.x > width - marginX)
      fractionX = (autoScrollPointer.x - (width - marginX)) / marginX;
    else if (autoScrollPointer.x < marginX)
      fractionX = (autoScrollPointer.x - marginX) / marginX;
    if (autoScrollPointer.y > height - marginY)
      fractionY = (autoScrollPointer.y - (height - marginY)) / marginY;
    else if (autoScrollPointer.y < marginY)
      fractionY = (autoScrollPointer.y - marginY) / marginY;
    const deltaX = Math.max(-maxSpeed, Math.min(maxSpeed, fractionX * maxSpeed));
    const deltaY = Math.max(-maxSpeed, Math.min(maxSpeed, fractionY * maxSpeed));
    if (deltaX === 0 && deltaY === 0)
      return;
    panX -= deltaX;
    panY -= deltaY;
    const item = nodeItemFor(autoScrollNodeId);
    if (item !== null)
      item.shiftDragOrigin(deltaX / zoom, deltaY / zoom);
  }

  function cancelPendingConnection() {
    pendingFlow = "";
    pendingNodeId = "";
    pendingPortId = "";
    pendingSignalKind = "";
  }

  function deleteSelectedConnection() {
    if (edgeModel === null)
      return;
    if (selectedConnectionId === "") {
      message = qsTr("Select a user connection to remove it.");
      return;
    }
    for (let i = 0; i < edgeModel.count; ++i) {
      const edge = edgeModel.get(i);
      if (edge.edgeId !== selectedConnectionId)
        continue;
      if (edge.readOnly) {
        message = edge.automatic === false ? qsTr("This connection is locked and cannot be removed.") : qsTr("Automatic serial routing is read-only.");
        return;
      }
      selectedConnectionId = "";
      projectDisconnectRequested(edge.sourcePort, edge.destinationPort);
      return;
    }
    selectedConnectionId = "";
  }

  function edgeIsDeletable(connectionId: string): bool {
    if (edgeModel === null)
      return false;
    for (let i = 0; i < edgeModel.count; ++i) {
      const edge = edgeModel.get(i);
      if (edge.edgeId === connectionId)
        return edge.readOnly !== true;
    }
    return false;
  }

  function findNodeIndex(nodeId: string): int {
    if (nodeModel === null)
      return -1;
    for (let i = 0; i < nodeModel.count; ++i) {
      if (nodeModel.get(i).nodeId === nodeId)
        return i;
    }
    return -1;
  }

  function fitToContent() {
    if (width <= 0 || height <= 0 || nodeModel === null)
      return;
    if (nodeModel.count === 0) {
      panX = 0;
      panY = 0;
      zoom = defaultZoom;
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

    const margin = 48;
    const contentWidth = maxX - minX + margin * 2;
    const contentHeight = maxY - minY + margin * 2;
    zoom = Math.max(minimumZoom, Math.min(1.25, (width - 32) / contentWidth, (height - 32) / contentHeight));
    panX = (width - (maxX - minX) * zoom) / 2 - minX * zoom;
    panY = (height - (maxY - minY) * zoom) / 2 - minY * zoom;
  }

  function moveNode(nodeId: string, x: real, y: real) {
    const index = findNodeIndex(nodeId);
    if (index < 0)
      return;
    const bound = 100000;
    nodeModel.setProperty(index, "nodeX", Math.max(-bound, Math.min(bound, x)));
    nodeModel.setProperty(index, "nodeY", Math.max(-bound, Math.min(bound - nodeHeight(nodeModel.get(index)), y)));
    ++layoutRevision;
  }

  // Tracks the modulation knob hovered in a node so the matching edge can
  // render highlighted.
  function setConnectionHighlight(sourcePortId: string, destinationPortId: string, hovered: bool): void {
    highlightedSourcePort = hovered ? sourcePortId : "";
    highlightedDestinationPort = hovered ? destinationPortId : "";
  }

  function nodeHeight(node: var): real {
    const inputs = JSON.parse(node.inputsJson);
    const outputs = JSON.parse(node.outputsJson);
    const parameters = JSON.parse(node.parametersJson);
    const editorBody = node.editorBodyHeight ?? 0;
    return headerHeight + Math.max(inputs.length + parameters.length, outputs.length) * portHeight + 8 + editorBody;
  }

  function portCenter(nodeId: string, portId: string, output: bool): point {
    const nodeIndex = findNodeIndex(nodeId);
    if (nodeIndex < 0)
      return Qt.point(0, 0);
    const node = nodeModel.get(nodeIndex);
    const ports = JSON.parse(output ? node.outputsJson : node.inputsJson);
    // The offsets land on the port dots' centers (6 margin + half the dot
    // width; parameter dots are wider than signal dots).
    for (let i = 0; i < ports.length; ++i) {
      if (ports[i].id === portId) {
        return Qt.point(node.nodeX + (output ? nodeWidth - 10 : 10), node.nodeY + headerHeight + i * portHeight + portHeight / 2);
      }
    }
    if (!output) {
      const parameters = JSON.parse(node.parametersJson);
      for (let i = 0; i < parameters.length; ++i) {
        if (parameters[i].id === portId) {
          return Qt.point(node.nodeX + 11, node.nodeY + headerHeight + (ports.length + i) * portHeight + portHeight / 2);
        }
      }
    }
    // Processor-level edges attach to the node's side edges: the fallback is
    // reached for the processor pseudo port, which names the processor
    // itself rather than one of its ports.
    return Qt.point(
      node.nodeX + (output ? nodeWidth : 0),
      node.nodeY + headerHeight / 2);
  }

  function beginConnection(nodeId: string, port: var) {
    forceActiveFocus();
    selectedNodeId = nodeId;
    selectedConnectionId = "";
    pendingFlow = port.flow;
    pendingNodeId = nodeId;
    pendingPortId = port.id;
    pendingSignalKind = port.kind;
    pendingPoint = portCenter(nodeId, port.id, port.flow === "output");
    message = qsTr("Drag to a compatible %1, or press Escape to cancel.").arg(port.flow === "output" ? qsTr("input") : qsTr("output"));
    refreshPendingConnection();
  }

  function completeConnection(nodeId: string, port: var) {
    if (!hasPendingConnection)
      return;

    const sourceNode = pendingFlow === "output" ? pendingNodeId : nodeId;
    const sourcePort = pendingFlow === "output" ? pendingPortId : port.id;
    const destinationNode = pendingFlow === "output" ? nodeId : pendingNodeId;
    const destinationPort = pendingFlow === "output" ? port.id : pendingPortId;
    const parameterMenuTarget = port.kind === "parameter" && port.transient === true;
    if (pendingFlow === "output" && pendingSignalKind === "cv"
        && parameterMenuTarget) {
      const sourcePortId = pendingPortId;
      cancelPendingConnection();
      selectedConnectionId = "";
      parameterConnectionRequested(nodeId, sourcePortId);
      message = qsTr("Choose a parameter to modulate.");
      return;
    }
    let failure = "";
    // Parameter slots accept CV sources; every other port requires the same
    // signal kind as the pending connection.
    const targetMatchesPendingKind = port.kind === "parameter"
      ? pendingSignalKind === "cv"
      : port.kind === pendingSignalKind;
    if (pendingNodeId === nodeId)
      failure = qsTr("Choose a port on another node.");
    else if (port.flow === pendingFlow)
      failure = qsTr("Choose a port in the opposite direction.");
    else if (!targetMatchesPendingKind)
      failure = qsTr("Signal types must match.");

    if (failure === "") {
      if (edgeModel !== null) {
        for (let i = 0; i < edgeModel.count; ++i) {
          const edge = edgeModel.get(i);
          if (edge.sourceNode === sourceNode && edge.sourcePort === sourcePort && edge.destinationNode === destinationNode && edge.destinationPort === destinationPort) {
            cancelPendingConnection();
            selectedConnectionId = "";
            message = "";
            return;
          }
        }
      }
    }

    if (failure !== "") {
      cancelPendingConnection();
      message = failure;
      return;
    }

    projectConnectRequested(sourcePort, destinationPort);
    cancelPendingConnection();
    selectedConnectionId = "";
  }

  function nodeAtWorldPosition(worldPoint: point): string {
    for (let i = 0; i < nodeRepeater.count; ++i) {
      const nodeItem = nodeRepeater.itemAt(i) as ModularNode;
      if (nodeItem === null)
        continue;
      if (worldPoint.x >= nodeItem.x && worldPoint.x <= nodeItem.x + nodeItem.width
          && worldPoint.y >= nodeItem.y && worldPoint.y <= nodeItem.y + nodeItem.height)
        return nodeItem.nodeId;
    }
    return "";
  }

  function portAtWorldPosition(worldPoint: point): var {
    for (let i = 0; i < nodeRepeater.count; ++i) {
      const nodeItem = nodeRepeater.itemAt(i) as ModularNode;
      if (nodeItem === null)
        continue;
      const port = nodeItem.portAtWorldPosition(worldPoint);
      if (port !== null)
        return port;
    }
    return null;
  }

  function portClicked(nodeId: string, port: var) {
    if (!hasPendingConnection) {
      if (port.kind === "parameter") {
        message = qsTr("Drag a CV output here to modulate a parameter.");
        return;
      }
      beginConnection(nodeId, port);
      return;
    }
    if (pendingNodeId === nodeId && pendingPortId === port.id) {
      cancelPendingConnection();
      message = qsTr("Connection cancelled.");
      return;
    }
    if (port.flow === pendingFlow) {
      beginConnection(nodeId, port);
      return;
    }
    completeConnection(nodeId, port);
  }

  function portConnectionStarted(nodeId: string, port: var, worldPoint: point) {
    beginConnection(nodeId, port);
    pendingPoint = worldPoint;
    refreshPendingConnection();
  }

  function portConnectionMoved(worldPoint: point) {
    if (!hasPendingConnection)
      return;
    pendingPoint = worldPoint;
    refreshPendingConnection();
  }

  function portConnectionFinished(worldPoint: point) {
    if (!hasPendingConnection)
      return;
    pendingPoint = worldPoint;
    const target = portAtWorldPosition(worldPoint);
    if (target === null) {
      const nodeId = nodeAtWorldPosition(worldPoint);
      if (nodeId !== "" && pendingFlow === "output" && pendingSignalKind === "cv") {
        const sourcePortId = pendingPortId;
        cancelPendingConnection();
        parameterConnectionRequested(nodeId, sourcePortId);
        return;
      }
      cancelPendingConnection();
      message = qsTr("Connection cancelled.");
      return;
    }
    completeConnection(target.nodeId, target.port);
  }

  function refreshCableEndpoints() {
    refreshPendingConnection();
  }

  function refreshPendingConnection() {
    if (!hasPendingConnection)
      return;
    pendingCable.sourcePoint = portCenter(pendingNodeId, pendingPortId, pendingFlow === "output");
    pendingCable.destinationPoint = pendingPoint;
  }

  function removeConnection(connectionId: string) {
    selectedConnectionId = connectionId;
    deleteSelectedConnection();
  }

  function selectConnection(connectionId: string, readOnly: bool, automatic: bool) {
    forceActiveFocus();
    selectedConnectionId = connectionId;
    selectedNodeId = "";
    message = readOnly ? (automatic ? qsTr("Automatic serial routing is read-only.") : qsTr("This connection is locked and cannot be removed.")) : qsTr("Connection selected. Right-click or press Delete to disconnect.");
  }

  function selectNode(nodeId: string) {
    forceActiveFocus();
    selectedNodeId = nodeId;
    selectedConnectionId = "";
    message = "";
    nodeSelected(nodeId);
  }

  function signalColor(kind: string): color {
    switch (kind) {
    case "midi":
      return ZrythmTheme.celestialBlueColor;
    case "cv":
      return ZrythmTheme.jonquilYellowColor;
    case "parameter":
      return ZrythmTheme.primaryColor;
    case "neutral":
      return ZrythmTheme.placeholderTextColor;
    default:
      return ZrythmTheme.springGreen;
    }
  }

  function centerOn(worldPoint: point) {
    panX = width / 2 - worldPoint.x * zoom;
    panY = height / 2 - worldPoint.y * zoom;
  }

  function worldCenter(): point {
    return Qt.point((width / 2 - panX) / zoom, (height / 2 - panY) / zoom);
  }

  function zoomAtCenter(newZoom: real) {
    zoomAtPoint(newZoom, Qt.point(width / 2, height / 2));
  }

  function zoomAtPoint(newZoom: real, canvasPoint: point) {
    const worldX = (canvasPoint.x - panX) / zoom;
    const worldY = (canvasPoint.y - panY) / zoom;
    zoom = Math.max(minimumZoom, Math.min(maximumZoom, newZoom));
    panX = canvasPoint.x - worldX * zoom;
    panY = canvasPoint.y - worldY * zoom;
  }

  activeFocusOnTab: true
  clip: true

  onLayoutRevisionChanged: refreshCableEndpoints()
  onNodeModelChanged: {
    cancelPendingConnection();
    selectedNodeId = "";
    selectedConnectionId = "";
    ++layoutRevision;
    if (autoFitOnModelChange)
      Qt.callLater(fitToContent);
  }

  Connections {
    function onModelReset() {
      ++root.layoutRevision;
    }
    function onRowsInserted() {
      ++root.layoutRevision;
    }
    function onRowsRemoved() {
      ++root.layoutRevision;
    }

    target: root.nodeModel
  }

  Rectangle {
    anchors.fill: parent
    color: ZrythmTheme.alternateBackgroundColor

    ModularGrid {
      anchors.fill: parent
      panX: root.panX
      panY: root.panY
      zoom: root.zoom
    }

    TapHandler {
      onTapped: {
        root.forceActiveFocus();
        if (root.hasPendingConnection) {
          root.cancelPendingConnection();
          root.message = qsTr("Connection cancelled.");
        } else {
          root.selectedNodeId = "";
          root.selectedConnectionId = "";
        }
      }
    }

    DragHandler {
      id: panHandler

      property point startPan

      acceptedButtons: Qt.LeftButton | Qt.MiddleButton
      cursorShape: active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
      target: null

      onActiveChanged: {
        if (active)
          startPan = Qt.point(root.panX, root.panY);
      }
      onActiveTranslationChanged: {
        if (active) {
          root.panX = startPan.x + activeTranslation.x;
          root.panY = startPan.y + activeTranslation.y;
        }
      }
    }
  }

  Item {
    id: world

    height: root.worldHeight
    scale: root.zoom
    transformOrigin: Item.TopLeft
    width: root.worldWidth
    x: root.panX
    y: root.panY

    Repeater {
      id: connectionRepeater

      model: root.edgeModel

      delegate: ModularConnection {
        id: cable

        required property bool automatic
        required property string destinationNode
        required property string destinationPort
        required property string edgeId
        required property bool readOnly
        required property string signalKind
        required property string sourceNode
        required property string sourcePort

        // Endpoint positions are computed from the node and port geometry.
        property point destinationPoint
        property point sourcePoint

        function refreshEndpoints() {
          sourcePoint = root.portCenter(sourceNode, sourcePort, true);
          destinationPoint = root.portCenter(destinationNode, destinationPort, false);
        }

        automaticCable: automatic
        cableColor: root.signalColor(signalKind)
        highlighted: root.highlightedSourcePort !== ""
          && root.highlightedSourcePort === cable.sourcePort
          && root.highlightedDestinationPort === cable.destinationPort
        connectionId: edgeId
        deletable: !readOnly
        destinationX: destinationPoint.x
        destinationY: destinationPoint.y
        interactive: true
        selected: root.selectedConnectionId === edgeId
        sourceX: sourcePoint.x
        sourceY: sourcePoint.y
        worldHeight: root.worldHeight
        worldWidth: root.worldWidth

        onClicked: (connectionId, automatic) => {
          root.selectConnection(connectionId, readOnly, automatic);
        }
        onDisconnectRequested: connectionId => root.removeConnection(connectionId)
        Component.onCompleted: refreshEndpoints()

        Connections {
          function onLayoutRevisionChanged() {
            cable.refreshEndpoints();
          }

          target: root
        }
      }
    }

    ModularConnection {
      id: pendingCable

      property point destinationPoint
      property point sourcePoint

      automaticCable: false
      cableColor: root.signalColor(root.pendingSignalKind)
      connectionId: ""
      destinationX: destinationPoint.x
      destinationY: destinationPoint.y
      interactive: false
      pending: true
      sourceX: sourcePoint.x
      sourceY: sourcePoint.y
      visible: root.hasPendingConnection
      worldHeight: root.worldHeight
      worldWidth: root.worldWidth
    }

    Repeater {
      id: nodeRepeater

      model: root.nodeModel

      delegate: ModularNode {
        canvasZoom: root.zoom
        cardWidth: root.nodeWidth
        headerHeight: root.headerHeight
        modularModel: root.modularModel
        parameterOperator: root.parameterOperator
        portHeight: root.portHeight
        connectionPending: root.hasPendingConnection
        linked: root.linkedNodeId === nodeId
        pendingFlow: root.pendingFlow
        pendingNodeId: root.pendingNodeId
        pendingPortId: root.pendingPortId
        pendingSignalKind: root.pendingSignalKind
        selected: root.selectedNodeId === nodeId
        worldItem: world

        onMoved: (nodeId, x, y) => root.moveNode(nodeId, x, y)
        onMoveFinished: (nodeId, x, y) => root.nodeMoveFinished(nodeId, x, y)
        onNodeDragStarted: nodeId => root.beginAutoScroll(nodeId)
        onNodeDragMoved: (nodeId, worldPointer) => root.updateAutoScroll(worldPointer)
        onNodeDragEnded: nodeId => root.endAutoScroll()
        onNodeMenuRequested: nodeId => root.nodeMenuRequested(nodeId)
        onParameterEditActiveChanged: active => root.parameterEditActiveChanged(active)
        onParameterMultiplierEdited: (sourcePortId, destinationPortId, value) => root.parameterMultiplierEdited(sourcePortId, destinationPortId, value)
        onModulationSourceHovered: (sourcePortId, destinationPortId, hovered) => root.setConnectionHighlight(sourcePortId, destinationPortId, hovered)
        onPortClicked: (nodeId, port) => root.portClicked(nodeId, port)
        onPortConnectionCancelled: root.cancelPendingConnection()
        onPortConnectionFinished: worldPoint => root.portConnectionFinished(worldPoint)
        onPortConnectionMoved: worldPoint => root.portConnectionMoved(worldPoint)
        onPortConnectionStarted: (nodeId, port, worldPoint) => root.portConnectionStarted(nodeId, port, worldPoint)
        onSelectionRequested: nodeId => root.selectNode(nodeId)
      }
    }
  }

  ModularMinimap {
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 16
    anchors.right: parent.right
    anchors.rightMargin: 16
    headerHeight: root.headerHeight
    height: 130
    nodeModel: root.nodeModel
    nodeWidth: root.nodeWidth
    panX: root.panX
    panY: root.panY
    portHeight: root.portHeight
    selectedNodeId: root.selectedNodeId
    viewportHeight: root.height
    viewportWidth: root.width
    visible: root.width >= 480 && root.height >= 320
    width: 190
    zoom: root.zoom

    onNavigationRequested: worldCenter => root.centerOn(worldCenter)
  }

  Timer {
    id: autoScrollTimer

    interval: 16
    repeat: true

    onTriggered: root.autoScrollTick()
  }

  HoverHandler {
    onPointChanged: {
      if (root.hasPendingConnection) {
        root.pendingPoint = world.mapFromItem(root, point.position);
        pendingCable.destinationPoint = root.pendingPoint;
      }
    }
  }

  WheelHandler {
    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    target: null

    onWheel: event => {
      if (event.angleDelta.y === 0)
        return;
      const factor = event.angleDelta.y > 0 ? 1.15 : 1 / 1.15;
      root.zoomAtPoint(root.zoom * factor, point.position);
    }
  }

  Shortcut {
    enabled: root.visible
    sequence: "F"

    onActivated: root.fitToContent()
  }

  Shortcut {
    enabled: root.visible && root.internalDeleteShortcutEnabled
    sequence: "Delete"

    onActivated: root.deleteSelectedConnection()
  }

  Shortcut {
    enabled: root.visible
    sequence: "Escape"

    onActivated: {
      root.cancelPendingConnection();
      root.selectedConnectionId = "";
      root.selectedNodeId = "";
      root.message = "";
    }
  }
}
