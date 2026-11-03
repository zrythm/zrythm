// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Shapes
import Zrythm
import ZrythmStyle

Item {
  id: root

  required property bool automaticCable
  property color cableColor: ZrythmTheme.springGreen
  required property string connectionId
  property bool deletable: false
  required property real destinationX
  required property real destinationY
  required property bool interactive
  property bool pending: false
  property bool selected: false
  required property real sourceX
  required property real sourceY
  required property real worldHeight
  required property real worldWidth

  readonly property real curveOffset: Math.max(40, Math.abs(destinationX - sourceX) * 0.45)
  readonly property real middleX: (sourceX + destinationX) / 2
  readonly property real middleY: (sourceY + destinationY) / 2
  readonly property string readOnlyText: automaticCable ? qsTr("Automatic serial routing — read-only") : qsTr("Project connection — read-only preview")
  readonly property string userConnectionText: qsTr("Right-click or press Delete to disconnect")

  signal clicked(string connectionId, bool automatic)
  signal disconnectRequested(string connectionId)

  height: worldHeight
  width: worldWidth

  Shape {
    // The curve renderer draws cables with native antialiasing and keeps
    // them smooth when the canvas is zoomed.
    preferredRendererType: Shape.CurveRenderer

    anchors.fill: parent

    ShapePath {
      capStyle: ShapePath.RoundCap
      dashPattern: [3, 3]
      fillColor: "transparent"
      startX: root.sourceX
      startY: root.sourceY
      strokeColor: root.pending || root.selected ? ZrythmTheme.primaryColor : (root.automaticCable ? ZrythmTheme.placeholderTextColor : root.cableColor)
      strokeStyle: root.automaticCable ? ShapePath.DashLine : ShapePath.SolidLine
      strokeWidth: root.selected ? 2.8 : (root.automaticCable ? 1.4 : 2.2)

      PathCubic {
        control1X: root.sourceX + root.curveOffset
        control1Y: root.sourceY
        control2X: root.destinationX - root.curveOffset
        control2Y: root.destinationY
        x: root.destinationX
        y: root.destinationY
      }
    }
  }

  Item {
    id: hitArea

    Accessible.name: root.deletable ? root.userConnectionText : root.readOnlyText
    Accessible.role: Accessible.Button
    activeFocusOnTab: root.interactive
    height: 18
    rotation: Math.atan2(root.destinationY - root.sourceY, root.destinationX - root.sourceX) * 180 / Math.PI
    visible: root.interactive
    width: Math.min(80, Math.max(24, Math.hypot(root.destinationX - root.sourceX, root.destinationY - root.sourceY)))
    x: root.middleX - width / 2
    y: root.middleY - height / 2

    onActiveFocusChanged: {
      if (activeFocus)
        root.clicked(root.connectionId, root.automaticCable);
    }

    Keys.onPressed: event => {
      if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
        root.clicked(root.connectionId, root.automaticCable);
        event.accepted = true;
      }
    }

    HoverHandler {
      id: hoverHandler
    }

    TapHandler {
      onTapped: {
        hitArea.forceActiveFocus();
        root.clicked(root.connectionId, root.automaticCable);
      }
    }

    TapHandler {
      acceptedButtons: Qt.RightButton
      enabled: root.deletable

      onTapped: {
        root.clicked(root.connectionId, root.automaticCable);
        connectionMenu.popup();
      }
    }

    ToolTip {
      text: root.deletable ? root.userConnectionText : root.readOnlyText
      visible: hoverHandler.hovered
    }
  }

  Menu {
    id: connectionMenu

    MenuItem {
      text: qsTr("Disconnect")

      onTriggered: root.disconnectRequested(root.connectionId)
    }
  }

  ToolButton {
    id: disconnectButton

    Accessible.name: qsTr("Disconnect connection")
    activeFocusOnTab: root.deletable && root.interactive
    height: 20
    text: "×"
    visible: root.deletable && (root.selected || hoverHandler.hovered || hitArea.activeFocus || disconnectButton.hovered || disconnectButton.activeFocus)
    width: 20
    x: root.middleX - width / 2
    y: root.middleY - height / 2

    onClicked: root.disconnectRequested(root.connectionId)

    ToolTip {
      text: qsTr("Disconnect")
      visible: disconnectButton.hovered
    }
  }

  Rectangle {
    Accessible.ignored: true
    border.color: ZrythmTheme.alternateBackgroundColor
    border.width: 1
    color: ZrythmTheme.cardColor
    height: 18
    radius: 4
    visible: root.interactive && !root.deletable && (root.selected || hoverHandler.hovered || hitArea.activeFocus)
    width: 20
    x: root.middleX - width / 2
    y: root.middleY - height / 2

    ColorImage {
      anchors.centerIn: parent
      color: ZrythmTheme.textColor
      height: 12
      source: ResourceManager.getIconUrl("gnome-icon-library", "padlock2-symbolic.svg")
      sourceSize: Qt.size(12, 12)
      width: 12
    }
  }
}
