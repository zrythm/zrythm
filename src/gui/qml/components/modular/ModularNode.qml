// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle

Control {
  id: root

  required property string accentKind
  required property real canvasZoom
  required property real cardWidth
  property bool connectionPending: false
  required property int headerHeight
  required property bool hasModulatableParameters
  required property string inputsJson
  required property string nodeId
  required property string nodeSubtitle
  required property string nodeTitle
  required property string nodeType
  required property real nodeX
  required property real nodeY
  required property string outputsJson
  required property string pendingFlow
  property string pendingNodeId: ""
  property string pendingPortId: ""
  property string pendingSignalKind: ""
  required property int portHeight
  property bool linked: false
  property bool selected: false
  required property string parametersJson
  required property Item worldItem

  readonly property var inputPorts: JSON.parse(inputsJson)
  readonly property var outputPorts: JSON.parse(outputsJson)
  readonly property var parameterPorts: JSON.parse(parametersJson)
  readonly property bool showsParameterDropTarget: connectionPending
    && pendingSignalKind === "cv"
    && pendingFlow === "output"
    && pendingNodeId !== nodeId
    && hasModulatableParameters
  // Counts the transient "Parameter…" drop-target row: the rendered node is
  // one row taller than the canvas' nodeHeight() geometry while a CV
  // connection hovers it.
  readonly property int leftPortCount: inputPorts.length + parameterPorts.length + (showsParameterDropTarget ? 1 : 0)
  readonly property var ports: {
    const leftPorts = portsWithFlow(inputPorts, "input");
    parameterPorts.forEach((port, index) => {
      leftPorts.push(Object.assign({}, port, {
        "flow": "input",
        "row": inputPorts.length + index
      }));
    });
    if (showsParameterDropTarget) {
      leftPorts.push({
        "flow": "input",
        "id": "parameter-menu",
        "kind": "parameter",
        "label": qsTr("Parameter…"),
        "row": leftPorts.length,
        "transient": true
      });
    }
    return leftPorts.concat(portsWithFlow(outputPorts, "output"));
  }

  signal moved(string nodeId, real x, real y)
  signal moveFinished(string nodeId, real x, real y)
  signal nodeDragStarted(string nodeId)
  signal nodeDragMoved(string nodeId, point worldPointer)
  signal nodeDragEnded(string nodeId)
  signal parameterEditActiveChanged(bool active)
  signal parameterMultiplierEdited(string sourcePortId, string destinationPortId, real value)
  signal portClicked(string nodeId, var port)
  signal portConnectionCancelled
  signal portConnectionFinished(point worldPoint)
  signal portConnectionMoved(point worldPoint)
  signal portConnectionStarted(string nodeId, var port, point worldPoint)
  signal selectionRequested(string nodeId)

  function isPendingSource(port: var): bool {
    return connectionPending && pendingNodeId === nodeId && pendingPortId === port.id;
  }

  function shiftDragOrigin(dxWorld: real, dyWorld: real) {
    if (!moveHandler.active)
      return;
    moveHandler.startPosition = Qt.point(moveHandler.startPosition.x + dxWorld, moveHandler.startPosition.y + dyWorld);
    root.moved(root.nodeId, moveHandler.startPosition.x + moveHandler.activeTranslation.x / root.canvasZoom, moveHandler.startPosition.y + moveHandler.activeTranslation.y / root.canvasZoom);
  }

  function isPendingTarget(port: var): bool {
    if (port.kind === "parameter")
      return connectionPending && pendingSignalKind === "cv"
        && pendingFlow === "output" && pendingNodeId !== nodeId;
    return connectionPending && port.flow !== pendingFlow && port.kind === pendingSignalKind && pendingNodeId !== nodeId;
  }

  function portAtWorldPosition(worldPoint: point): var {
    for (let i = 0; i < portRepeater.count; ++i) {
      const portItem = portRepeater.itemAt(i);
      if (portItem === null)
        continue;
      const localPoint = portItem.mapFromItem(worldItem, worldPoint);
      if (localPoint.x >= 0 && localPoint.x < portItem.width && localPoint.y >= 0 && localPoint.y < portItem.height) {
        return {
          "nodeId": nodeId,
          "port": root.ports[i]
        };
      }
    }
    return null;
  }

  function portsWithFlow(portList: var, flow: string): var {
    return portList.map((port, index) => Object.assign({}, port, {
        "flow": flow,
        "row": index
      }));
  }

  // Parameter slots use the full node width except on rows shared with an
  // output port, where they stay in the left column.
  function portSlotWidth(port: var): real {
    if (port.kind === "parameter" && port.row >= outputPorts.length)
      return cardWidth;
    return cardWidth / 2;
  }

  function portToolTip(port: var): string {
    if (port.kind === "parameter") {
      if (port.transient === true)
        return qsTr("Drop to choose a parameter");
      if (isPendingTarget(port))
        return qsTr("Drop to modulate this parameter");
      if (connectionPending)
        return qsTr("This parameter is not compatible");
      return qsTr("Parameter modulation");
    }
    if (isPendingSource(port))
      return qsTr("Connection start");
    if (isPendingTarget(port))
      return qsTr("Drop to connect");
    if (connectionPending && port.flow === pendingFlow)
      return qsTr("Start a connection from this port instead");
    if (connectionPending)
      return qsTr("This port is not compatible");
    return qsTr("Drag to create a connection");
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

  Accessible.name: nodeTitle
  Accessible.role: Accessible.Grouping
  activeFocusOnTab: true
  height: headerHeight + Math.max(leftPortCount, outputPorts.length) * portHeight + 8
  hoverEnabled: true
  padding: 0
  width: cardWidth
  x: nodeX
  y: nodeY

  background: Rectangle {
    border.color: root.selected || root.linked || root.activeFocus ? palette.highlight : ZrythmTheme.alternateBackgroundColor
    border.width: root.selected || root.linked ? 2 : 1
    color: ZrythmTheme.cardColor
    radius: 6
  }

  Item {
    id: header

    height: root.headerHeight
    width: root.width

    Rectangle {
      anchors.horizontalCenter: parent.horizontalCenter
      color: root.signalColor(root.accentKind)
      height: 3
      width: parent.width - 12
    }

    ColumnLayout {
      anchors.left: parent.left
      anchors.leftMargin: 12
      anchors.right: parent.right
      anchors.rightMargin: 12
      anchors.verticalCenter: parent.verticalCenter
      spacing: 1

      RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
          Layout.fillWidth: true
          color: ZrythmTheme.textColor
          elide: Text.ElideRight
          font: ZrythmTheme.semiBoldTextFont
          text: root.nodeTitle
        }

        Label {
          color: ZrythmTheme.placeholderTextColor
          elide: Text.ElideRight
          font: ZrythmTheme.xSmallTextFont
          text: root.nodeType
          visible: text !== ""
        }
      }

      Label {
        Layout.fillWidth: true
        color: ZrythmTheme.placeholderTextColor
        elide: Text.ElideRight
        font: ZrythmTheme.xSmallTextFont
        text: root.nodeSubtitle
        visible: text !== ""
      }
    }

    TapHandler {
      onTapped: root.selectionRequested(root.nodeId)
    }

    DragHandler {
      id: moveHandler

      property point startPosition

      acceptedButtons: Qt.LeftButton
      cursorShape: Qt.ClosedHandCursor
      dragThreshold: 4
      grabPermissions: PointerHandler.CanTakeOverFromAnything | PointerHandler.ApprovesCancellation
      target: null

      onActiveChanged: {
        if (active) {
          startPosition = Qt.point(root.nodeX, root.nodeY);
          root.selectionRequested(root.nodeId);
          root.nodeDragStarted(root.nodeId);
        } else {
          root.nodeDragEnded(root.nodeId);
          root.moveFinished(root.nodeId, root.nodeX, root.nodeY);
        }
      }
      onActiveTranslationChanged: {
        if (active) {
          root.moved(root.nodeId, startPosition.x + activeTranslation.x / root.canvasZoom, startPosition.y + activeTranslation.y / root.canvasZoom);
          root.nodeDragMoved(root.nodeId, root.worldItem.mapFromItem(header, centroid.position));
        }
      }
    }
  }

  Repeater {
    id: portRepeater

    model: root.ports

    delegate: Button {
      id: portButton

      required property var modelData

      flat: true
      font: ZrythmTheme.xSmallTextFont
      height: root.portHeight
      highlighted: root.isPendingSource(modelData) || root.isPendingTarget(modelData)
      leftPadding: 16
      rightPadding: portButton.modelData.kind === "parameter" ? parameterKnobs.implicitWidth + 20 : 16
      text: modelData.label
      width: root.portSlotWidth(modelData)
      x: modelData.flow === "output" ? root.cardWidth - width : 0
      y: root.headerHeight + modelData.row * root.portHeight

      Keys.onPressed: event => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
          root.portClicked(root.nodeId, modelData);
          event.accepted = true;
        }
      }

      // The connection DragHandler only takes the grab once the drag
      // threshold is exceeded, so plain clicks arrive here.
      onClicked: root.portClicked(root.nodeId, modelData)

      Accessible.name: qsTr("%1, %2").arg(root.nodeTitle).arg(modelData.label)

      DragHandler {
        id: connectionDragHandler

        property bool cancelled: false
        property point lastWorldPoint

        function currentWorldPoint(): point {
          return root.worldItem.mapFromItem(portButton, centroid.position);
        }

        acceptedButtons: Qt.LeftButton
        dragThreshold: 4
        enabled: portButton.modelData.kind !== "parameter"
        grabPermissions: PointerHandler.CanTakeOverFromAnything | PointerHandler.ApprovesCancellation
        target: null

        onActiveChanged: {
          if (active) {
            cancelled = false;
            lastWorldPoint = currentWorldPoint();
            root.portConnectionStarted(root.nodeId, portButton.modelData, lastWorldPoint);
          } else {
            if (!cancelled)
              root.portConnectionFinished(lastWorldPoint);
            cancelled = false;
          }
        }
        onActiveTranslationChanged: {
          if (active) {
            lastWorldPoint = currentWorldPoint();
            root.portConnectionMoved(lastWorldPoint);
          }
        }
        onCanceled: {
          cancelled = true;
          root.portConnectionCancelled();
        }
      }

      Rectangle {
        Accessible.ignored: true
        anchors.left: portButton.modelData.flow === "input" ? parent.left : undefined
        anchors.margins: 6
        anchors.right: portButton.modelData.flow === "output" ? parent.right : undefined
        anchors.verticalCenter: parent.verticalCenter
        color: root.signalColor(portButton.modelData.kind)
        height: portButton.modelData.kind === "parameter" ? 6 : 8
        radius: portButton.modelData.kind === "audio" || portButton.modelData.kind === "parameter" ? 3 : 1
        rotation: portButton.modelData.kind === "cv" ? 45 : 0
        width: portButton.modelData.kind === "parameter" ? 10 : 8
      }

      // Modulation amount knobs, one per connection into the parameter
      // slot. With a single connection the current amount is shown as a
      // percentage next to the knob.
      Row {
        id: parameterKnobs

        readonly property var slotConnections: portButton.modelData.connections !== undefined ? portButton.modelData.connections : []

        anchors.right: portButton.modelData.flow === "input" ? portButton.right : undefined
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6
        visible: portButton.modelData.kind === "parameter" && parameterKnobs.slotConnections.length > 0

        Repeater {
          id: knobs

          model: parameterKnobs.slotConnections

          delegate: Row {
            id: connectionDelegate

            required property var modelData

            spacing: 2

            Knob {
              id: amountKnob

              // The full range is -100%…100%; new connections start at 100%.
              defaultValue: 1
              enabled: connectionDelegate.modelData.locked !== true
              flat: true
              maxValue: 1
              minValue: -1
              size: 20
              value: connectionDelegate.modelData.multiplier !== undefined ? connectionDelegate.modelData.multiplier : 0
              zero: 0

              onDraggingChanged: root.parameterEditActiveChanged(dragging)
              onValueChanged: root.parameterMultiplierEdited(connectionDelegate.modelData.sourcePortId, portButton.modelData.id, amountKnob.value)
            }

            Label {
              color: ZrythmTheme.placeholderTextColor
              font: ZrythmTheme.xSmallTextFont
              horizontalAlignment: Text.AlignRight
              text: Math.round(amountKnob.value * 100) + "%"
              verticalAlignment: Text.AlignVCenter
              visible: knobs.count === 1
              width: 38
            }
          }
        }
      }

      ToolTip {
        text: root.portToolTip(portButton.modelData)
      }
    }
  }
}
