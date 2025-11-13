// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import Zrythm

Control {
  id: root

  required property ArrangerObject arrangerObject
  readonly property alias down: dragArea.pressed
  property point hoveredPoint: Qt.point(0, 0)
  property bool isResizeLHovered: false
  property bool isResizeRHovered: false
  property bool isResizingL: false // simple flag to be turned off immediately after checked by the interested party (Arranger)
  property bool isResizingR: false // ditto
  required property bool isSelected
  required property UndoStack undoStack
  readonly property color objectColor: {
    let c = (arrangerObject.color && arrangerObject.color.useColor) ? arrangerObject.color.color : (track ? track.color : palette.button);
    if (root.isSelected)
      c = Style.getColorBlendedTowardsContrastByFactor(c, 1.1);

    return Style.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.visualFocus, root.down);
  }
  required property Track track

  signal selectionRequested(var mouse)

  function requestSelection(mouse: var) {
    selectionRequested(mouse);
  }

  focusPolicy: Qt.StrongFocus
  font: root.isSelected ? Style.arrangerObjectBoldTextFont : Style.arrangerObjectTextFont
  height: track.height
  hoverEnabled: true
  implicitWidth: 20

  ContextMenu.menu: Menu {
    MenuItem {
      text: qsTr("Test")

      onTriggered: console.log("Clicked")
    }
  }

  MouseArea {
    id: dragArea

    property real startX

    acceptedButtons: Qt.AllButtons
    anchors.fill: parent
    cursorShape: undefined // to be set by Arranger
    hoverEnabled: true
    propagateComposedEvents: true

    onPositionChanged: mouse => {
      mouse.accepted = false;
      root.hoveredPoint = Qt.point(mouse.x, mouse.y);
    }
    onPressed: mouse => {
      mouse.accepted = false;
    }
  }

  Loader {
    id: edgeHandlesLoader

    active: root.arrangerObject.bounds !== null
    anchors.fill: parent
    enabled: active
    visible: active
    z: 10

    sourceComponent: Item {
      anchors.fill: parent

      // Left resize handle
      Handle {
        id: leftHandle

        anchors.left: parent.left
        isLeft: true
      }

      // Right resize handle
      Handle {
        id: rightHandle

        anchors.right: parent.right
        isLeft: false
      }
    }
  }

  component Handle: Rectangle {
    id: handleRect

    required property bool isLeft

    bottomLeftRadius: isLeft ? Style.toolButtonRadius : 0
    bottomRightRadius: isLeft ? 0 : Style.toolButtonRadius
    color: Style.backgroundAppendColor
    height: parent.height
    topLeftRadius: isLeft ? Style.toolButtonRadius : 0
    topRightRadius: isLeft ? 0 : Style.toolButtonRadius
    visible: root.hovered || handleDragArea.pressed
    width: 5

    MouseArea {
      id: handleDragArea

      acceptedButtons: Qt.LeftButton
      anchors.fill: parent
      cursorShape: undefined // to be set by Arranger
      hoverEnabled: true
      propagateComposedEvents: true

      onEntered: {
        if (handleRect.isLeft) {
          root.isResizeLHovered = true;
        } else {
          root.isResizeRHovered = true;
        }
      }
      onExited: {
        if (handleRect.isLeft) {
          root.isResizeLHovered = false;
        } else {
          root.isResizeRHovered = false;
        }
      }
      onPositionChanged: mouse => {
        root.hoveredPoint = handleRect.mapToItem(root, Qt.point(mouse.x, mouse.y));
        mouse.accepted = false;
      }
      onPressed: mouse => {
        if (handleRect.isLeft) {
          root.isResizingL = true;
        } else {
          root.isResizingR = true;
        }
        mouse.accepted = false;
      }
    }
  }
}
