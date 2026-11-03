// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import Zrythm
import ZrythmGui
import ZrythmStyle

ColumnLayout {
  id: root

  required property ProjectSession session
  required property TrackSelectionModel trackSelectionModel

  readonly property ModularProjectModel modularModel: root.session.modularProjectModel
  readonly property int scopeIndex: root.modularModel.global ? 1 : 0
  readonly property string linkedTrackNodeId: root.modularModel.global
    && root.trackSelectionModel.hasSelection
    ? "track:" + root.modularModel.scopeTrackUuid
    : ""

  property var parameterMenuEntries: []
  property string parameterMenuSourcePortId: ""
  // While a slot-knob drag edits a connection, projection refreshes are
  // deferred so the dragged knob's delegate is not recreated mid-gesture.
  property bool connectionEditing: false
  property bool deferredModelRefresh: false

  readonly property string defaultSummary: {
    const nodeCount = canvas.nodeModel === null ? 0 : canvas.nodeModel.count;
    const edgeCount = canvas.edgeModel === null ? 0 : canvas.edgeModel.count;
    return qsTr("%1 · %2 nodes · %3 connections").arg(root.modularModel.scopeTitle).arg(nodeCount).arg(edgeCount);
  }

  function copyProjection(source: var, destination: var) {
    destination.clear();
    for (const entry of source)
      destination.append(entry);
  }

  function refreshNodesProjection() {
    copyProjection(root.modularModel.nodes, canvas.nodeModel);
  }

  function refreshEdgesProjection() {
    copyProjection(root.modularModel.edges, canvas.edgeModel);
  }

  function completeParameterConnection(portId: string) {
    const error = root.session.portConnectionOperator.connectPorts(root.parameterMenuSourcePortId, portId);
    canvas.message = error === "" ? qsTr("Connection created.") : error;
  }

  function requestParameterConnection(nodeId: string, sourcePortId: string) {
    const entries = root.modularModel.modulatableParameters(nodeId);
    if (entries.length === 0) {
      canvas.message = qsTr("This node has no modulatable parameters.");
      return;
    }
    root.parameterMenuSourcePortId = sourcePortId;
    root.parameterMenuEntries = entries;
    parameterMenu.popup();
  }

  function applySavedViewport(): bool {
    const saved = root.modularModel.viewport();
    if (saved.panX === undefined)
      return false;
    canvas.panX = saved.panX;
    canvas.panY = saved.panY;
    canvas.zoom = saved.zoom;
    return true;
  }

  function saveViewport() {
    root.modularModel.setViewport(canvas.panX, canvas.panY, canvas.zoom);
  }

  function syncScopeTrackFromSelection() {
    const trackUuid = root.trackSelectionModel.currentOrFirstSelectedUuidString();
    if (trackUuid !== "")
      root.modularModel.setScopeTrack(trackUuid);
  }

  function selectTrackFromGraphNode(nodeId: string) {
    if (!nodeId.startsWith("track:"))
      return;
    root.trackSelectionModel.selectTracksByUuidStrings(
      [nodeId.substring("track:".length)]);
  }

  spacing: 0

  Component.onCompleted: {
    root.syncScopeTrackFromSelection();
    copyProjection(root.modularModel.nodes, displayNodes);
    copyProjection(root.modularModel.edges, displayEdges);
    if (!applySavedViewport())
      Qt.callLater(canvas.fitToContent);
  }

  Connections {
    function onEdgesChanged() {
      if (root.connectionEditing) {
        root.deferredModelRefresh = true;
        return;
      }
      root.refreshEdgesProjection();
    }
    function onNodesChanged() {
      if (root.connectionEditing) {
        root.deferredModelRefresh = true;
        return;
      }
      root.refreshNodesProjection();
      root.refreshEdgesProjection();
      if (!root.applySavedViewport())
        Qt.callLater(canvas.fitToContent);
    }
    function onScopeChanged() {
      root.applySavedViewport();
    }

    target: root.modularModel
  }

  Connections {
    function onCurrentChanged(current, previous) {
      root.syncScopeTrackFromSelection();
    }
    function onSelectionChanged(selected, deselected) {
      root.syncScopeTrackFromSelection();
    }

    target: root.trackSelectionModel
  }

  Connections {
    function onNodeMoveFinished(nodeId: string, x: real, y: real) {
      root.modularModel.setNodePosition(nodeId, x, y);
    }
    function onPanXChanged() {
      root.saveViewport();
    }
    function onPanYChanged() {
      root.saveViewport();
    }
    function onZoomChanged() {
      root.saveViewport();
    }
    function onParameterConnectionRequested(nodeId: string, sourcePortId: string) {
      root.requestParameterConnection(nodeId, sourcePortId);
    }
    function onProjectConnectRequested(sourcePortId: string, destinationPortId: string) {
      const error = root.session.portConnectionOperator.connectPorts(sourcePortId, destinationPortId);
      canvas.message = error === "" ? qsTr("Connection created.") : error;
    }
    function onProjectDisconnectRequested(sourcePortId: string, destinationPortId: string) {
      const error = root.session.portConnectionOperator.disconnectPorts(sourcePortId, destinationPortId);
      canvas.message = error === "" ? qsTr("Connection removed.") : error;
    }
    function onNodeSelected(nodeId: string) {
      root.selectTrackFromGraphNode(nodeId);
    }
    function onParameterMultiplierEdited(sourcePortId: string, destinationPortId: string, value: real) {
      const error = root.session.portConnectionOperator.setConnectionMultiplier(sourcePortId, destinationPortId, value);
      if (error !== "")
        canvas.message = error;
    }
    function onParameterEditActiveChanged(active: bool) {
      root.connectionEditing = active;
      if (!active && root.deferredModelRefresh) {
        root.deferredModelRefresh = false;
        root.refreshNodesProjection();
        root.refreshEdgesProjection();
      }
    }

    target: canvas
  }

  Menu {
    id: parameterMenu

    Instantiator {
      model: root.parameterMenuEntries

      delegate: MenuItem {
        required property var modelData

        text: modelData.label

        onTriggered: root.completeParameterConnection(modelData.portId)
      }

      onObjectAdded: (index, object) => parameterMenu.insertItem(index, object)
      onObjectRemoved: (index, object) => parameterMenu.removeItem(object)
    }
  }

  ListModel {
    id: displayNodes
  }

  ListModel {
    id: displayEdges
  }

  readonly property ViewContext viewContext: ViewContext {
    canCopy: false
    canDelete: canvas.hasDeletableSelection
    canPaste: false

    onDeleteRequested: canvas.deleteSelectedConnection()
  }

  ToolBar {
    Layout.fillWidth: true

    RowLayout {
      anchors.fill: parent
      spacing: 4

      TabBar {
        id: scopeBar

        Component.onCompleted: setCurrentIndex(root.modularModel.global ? 1 : 0)
        Layout.preferredHeight: 30
        Layout.preferredWidth: 160

        onCurrentIndexChanged: {
          const wantsGlobal = currentIndex === 1;
          if (wantsGlobal === root.modularModel.global)
            return;
          if (wantsGlobal)
            root.modularModel.showGlobal();
          else if (root.modularModel.scopeTrackUuid !== "")
            root.modularModel.showTrack(root.modularModel.scopeTrackUuid);
          else
            currentIndex = root.modularModel.global ? 1 : 0;
        }

        Connections {
          function onScopeChanged() {
            scopeBar.setCurrentIndex(root.modularModel.global ? 1 : 0);
          }

          target: root.modularModel
        }

        TabButton {
          id: trackScopeButton

          objectName: "trackScopeButton"
          text: qsTr("Tracks")
        }

        TabButton {
          id: globalScopeButton

          objectName: "globalScopeButton"
          text: qsTr("Global")
        }
      }

      Item {
        Layout.fillWidth: true
      }

      ToolButton {
        text: "−"

        onClicked: canvas.zoomAtCenter(canvas.zoom / 1.15)

        Accessible.name: qsTr("Zoom out")
      }

      Label {
        Layout.preferredWidth: 44
        font: ZrythmTheme.smallTextFont
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("%1%").arg(Math.round(canvas.zoom * 100))
      }

      ToolButton {
        text: "+"

        onClicked: canvas.zoomAtCenter(canvas.zoom * 1.15)

        Accessible.name: qsTr("Zoom in")
      }

      ToolButton {
        text: qsTr("Fit")

        onClicked: canvas.fitToContent()
      }

      ToolButton {
        objectName: "arrangeButton"
        text: qsTr("Auto-arrange")

        onClicked: {
          root.modularModel.arrangeAutomatically();
          Qt.callLater(canvas.fitToContent);
        }
      }
    }
  }

  RowLayout {
    Layout.fillHeight: true
    Layout.fillWidth: true
    spacing: 0

    Rectangle {
      Layout.fillHeight: true
      Layout.preferredWidth: root.scopeIndex === 0 ? 176 : 0
      color: ZrythmTheme.pageColor
      visible: root.scopeIndex === 0

      ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
          Layout.fillWidth: true
          Layout.leftMargin: 10
          Layout.topMargin: 8
          color: ZrythmTheme.placeholderTextColor
          font: ZrythmTheme.smallTextFont
          text: qsTr("Tracks")
        }

        ListView {
          id: trackList

          Layout.fillHeight: true
          Layout.fillWidth: true
          Layout.topMargin: 6
          clip: true
          model: root.session.project.tracklist.collection

          delegate: ItemDelegate {
            id: trackDelegate

            required property int index
            required property string trackName
            required property string trackUuidString

            highlighted: sidebarSelectionTracker.isSelected
            objectName: "trackDelegate-" + trackUuidString
            text: trackName
            width: ListView.view.width

            function selectionModelIndex(): var {
              return root.trackSelectionModel.getModelIndex(trackDelegate.index);
            }

            SelectionTracker {
              id: sidebarSelectionTracker

              modelIndexProvider: trackDelegate.selectionModelIndex
              selectionModel: root.trackSelectionModel
            }

            onClicked: root.trackSelectionModel.selectTracksByUuidStrings([trackUuidString])
          }
        }
      }
    }

    ModularCanvas {
      id: canvas

      Layout.fillHeight: true
      Layout.fillWidth: true
      autoFitOnModelChange: false
      edgeModel: displayEdges
      internalDeleteShortcutEnabled: false
      nodeModel: displayNodes
      objectName: "modularCanvas"
      linkedNodeId: root.linkedTrackNodeId
    }
  }

  Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 28
    color: ZrythmTheme.pageColor

    RowLayout {
      anchors.fill: parent
      anchors.leftMargin: 8
      anchors.rightMargin: 8
      spacing: 12

      Label {
        Layout.fillWidth: true
        color: ZrythmTheme.placeholderTextColor
        elide: Text.ElideRight
        font: ZrythmTheme.xSmallTextFont
        text: canvas.message !== "" ? canvas.message : root.defaultSummary
      }

      RowLayout {
        spacing: 4
        visible: root.width >= 850

        LegendLine {
          dashed: true
          lineColor: ZrythmTheme.placeholderTextColor
        }

        Label {
          font: ZrythmTheme.xSmallTextFont
          text: qsTr("Automatic (locked)")
        }
      }

      RowLayout {
        spacing: 4
        visible: root.width >= 850

        LegendLine {
          lineColor: ZrythmTheme.springGreen
        }

        Label {
          font: ZrythmTheme.xSmallTextFont
          text: qsTr("User connection")
        }
      }

      Label {
        color: ZrythmTheme.placeholderTextColor
        font: ZrythmTheme.xSmallTextFont
        text: qsTr("Automatic routing locked")
        visible: root.width >= 1100
      }
    }
  }

  component LegendLine: Shape {
    id: legendLine

    preferredRendererType: Shape.CurveRenderer

    property bool dashed: false
    property color lineColor: ZrythmTheme.springGreen

    implicitHeight: 14
    implicitWidth: 28

    ShapePath {
      capStyle: ShapePath.RoundCap
      dashPattern: [3, 3]
      fillColor: "transparent"
      startX: 2
      startY: 7
      strokeColor: legendLine.lineColor
      strokeStyle: legendLine.dashed ? ShapePath.DashLine : ShapePath.SolidLine
      strokeWidth: legendLine.dashed ? 1.4 : 2.2

      PathLine {
        x: 26
        y: 7
      }
    }
  }
}
