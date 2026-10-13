// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
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
  readonly property alias edgeHandles: edgeHandlesLoader
  // How the object's base color (its own color, else the track color)
  // maps to its on-screen color: Raw keeps it as-is, Content derives
  // the shared content ink (small elements — notes, curves,
  // waveforms), Label derives the clip-like tinted body (quiet spans
  // and label boxes — scale objects, markers, chord events,
  // tempo/time-signature objects) — DESIGN.md "Non-clip arranger
  // objects". States (hover, press, selection, muted) apply on top
  // of the derived color. objectAccentColor pairs a Label body with
  // a small content-ink anchor (marker pennants, scale ribbons).
  enum ColorRole { Raw, Content, Label }
  property int colorRole: ArrangerObjectBaseView.ColorRole.Raw
  property point hoveredPoint: Qt.point(0, 0)
  property bool isResizeLHovered: false
  property bool isResizeRHovered: false
  property bool isResizingL: false // simple flag to be turned off immediately after checked by the interested party (Arranger)
  property bool isResizingR: false // ditto
  required property bool isSelected
  // The object's color: its own color where set, else the track color
  readonly property color objectBaseColor:
    (arrangerObject.color && arrangerObject.color.useColor) ? arrangerObject.color.color : (track ? track.color : palette.button);
  // The role's derived colors in one pass; they depend only on the
  // base color and the theme, so state changes (hover, press,
  // selection, muted) re-run only the state pipeline below, never
  // the derivation. The accent ink is computed for Label objects,
  // which pair a quiet body with a small saturated anchor.
  readonly property var _roleColors: {
    if (colorRole === ArrangerObjectBaseView.ColorRole.Content)
      return { main: ZrythmTheme.contentColor (objectBaseColor, false) };
    if (colorRole === ArrangerObjectBaseView.ColorRole.Label) {
      const label = ZrythmTheme.labelColors (objectBaseColor, false);
      return {
        main: label.body,
        border: label.border,
        accent: ZrythmTheme.contentColor (objectBaseColor, false)
      };
    }
    return { main: objectBaseColor };
  }
  readonly property color objectDerivedColor: root._roleColors.main
  readonly property color objectBorderColor: root._roleColors.border ?? root.objectDerivedColor
  readonly property color objectColor: root._applyObjectStates (root.objectDerivedColor)
  // The content ink for the object's hue, run through the same state
  // pipeline as objectColor
  readonly property color objectAccentColor:
    root._applyObjectStates (root._roleColors.accent ?? root.objectDerivedColor)
  readonly property bool _isMuted: arrangerObject.mute?.muted ?? false
  required property Track track
  required property UndoStack undoStack

  function _applyObjectStates (arg: color): color {
    let c = arg;
    if (root.isSelected)
      c = ZrythmTheme.getColorBlendedTowardsContrastByFactor(c, 1.1);

    c = ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.visualFocus, root.down);

    if (root._isMuted)
      c = QmlUtils.saturate(c, 0.0);

    return c;
  }

  // Corner radius of the selection outline; types whose shape has corners
  // other than the default set this to match (e.g. half the height for the
  // fully-round automation point).
  property real selectionCornerRadius: ZrythmTheme.toolButtonRadius

  signal selectionRequested(var mouse)
  signal objectDoubleClicked

  function requestSelection(mouse: var) {
    selectionRequested(mouse);
  }

  focusPolicy: Qt.StrongFocus
  font: root.isSelected ? ZrythmTheme.arrangerObjectBoldTextFont : ZrythmTheme.arrangerObjectTextFont
  height: track.height
  hoverEnabled: true
  implicitWidth: 20

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

    active: root.arrangerObject.length !== null
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

  Rectangle {
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    anchors.left: parent.left
    border.color: root.palette.windowText
    border.width: 1
    color: "transparent"
    radius: root.selectionCornerRadius
    visible: root.isSelected
    z: 200
  }

  component Handle: Rectangle {
    id: handleRect

    required property bool isLeft

    bottomLeftRadius: isLeft ? ZrythmTheme.toolButtonRadius : 0
    bottomRightRadius: isLeft ? 0 : ZrythmTheme.toolButtonRadius
    color: ZrythmTheme.backgroundAppendColor
    height: parent.height
    topLeftRadius: isLeft ? ZrythmTheme.toolButtonRadius : 0
    topRightRadius: isLeft ? 0 : ZrythmTheme.toolButtonRadius
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
