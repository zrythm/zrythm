// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui
import ZrythmStyle

Arranger {
  id: root

  required property AudioClipEditor audioEditor
  readonly property real fadeInPx: root.region && root.region.fadeRange ? root.region.fadeRange.startOffset.ticks * root.ruler.pxPerTick : 0
  readonly property real fadeOutPx: root.region && root.region.fadeRange ? root.region.fadeRange.endOffset.ticks * root.ruler.pxPerTick : 0
  property QObjectPropertyOperator fadePropertyOperator: null
  readonly property AudioRegion region: clipEditor.region
  readonly property real regionWidth: root.region && root.region.bounds ? root.region.bounds.length.ticks * root.ruler.pxPerTick : 0
  readonly property real regionX: root.region ? root.region.position.ticks * root.ruler.pxPerTick : 0
  readonly property Track track: clipEditor.track

  function beginObjectCreation(coordinates: point): var {
    return null;
  }

  function getObjectHeight(obj): real {
    return 0;
  }

  function getObjectY(obj): real {
    return 0;
  }

  function moveSelectionsY(dy: real, prevY: real) {
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
  }

  editorSettings: audioEditor.editorSettings
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: Item {
    id: contentItem

    readonly property color fadeCurveColor: Qt.alpha(palette.text, 0.5)
    readonly property color fadeOverlayColor: Qt.alpha(palette.shadow, 0.25)

    anchors.fill: parent

    WaveformCanvas {
      id: waveformCanvas

      height: contentItem.height
      outlineColor: {
        const lighter = Qt.lighter(root.track.color, 1.4);
        return Qt.rgba(lighter.r, lighter.g, lighter.b, 200 / 255);
      }
      pxPerTick: root.ruler.pxPerTick
      region: root.region
      waveformColor: Qt.rgba(root.track.color.r, root.track.color.g, root.track.color.b, 80 / 255)
      width: root.regionWidth
      x: root.regionX
      y: 0
    }

    FadeOverlayControl {
      curveOptsObj: root.region ? root.region.fadeRange.fadeInCurveOpts : null
      curvinessAction: Arranger.ResizingUpFadeIn
      fadePx: fadeInPx
      fadeType: FadeOverlayCanvas.FadeIn
      offsetAction: Arranger.ResizingLFade
      offsetObj: root.region ? root.region.fadeRange.startOffset : null
      x: root.regionX
      y: 0
    }

    FadeOverlayControl {
      curveOptsObj: root.region ? root.region.fadeRange.fadeOutCurveOpts : null
      curvinessAction: Arranger.ResizingUpFadeOut
      fadePx: fadeOutPx
      fadeType: FadeOverlayCanvas.FadeOut
      offsetAction: Arranger.ResizingRFade
      offsetObj: root.region ? root.region.fadeRange.endOffset : null
      x: root.regionX + root.regionWidth - fadeOutPx
      y: 0
    }

    // Gain line (TODO: make it operational)
    Item {
      id: gainLine

      readonly property real gainY: root.region ? (1.0 - root.region.gain / 2.0) * height : 0

      height: parent.height
      visible: root.region != null
      width: root.regionWidth
      x: root.regionX
      y: 0

      Rectangle {
        color: palette.accent
        height: 2
        width: parent.width
        y: gainLine.gainY

        Label {
          color: palette.text
          font.pixelSize: 10
          text: (20 * Math.log10(root.region ? root.region.gain : 1.0)).toFixed(1) + " dB"
          y: -height - 2
        }
      }
    }
  }

  component FadeOverlayControl: Item {
    id: fadeOverlayControl

    required property CurveOptions curveOptsObj
    required property int curvinessAction
    required property real fadePx
    required property int fadeType
    required property int offsetAction
    required property AtomicPosition offsetObj

    height: contentItem.height
    width: Math.max(fadePx, 20)

    FadeOverlayCanvas {
      curveColor: contentItem.fadeCurveColor
      fadeType: fadeOverlayControl.fadeType
      height: parent.height
      hovered: curvinessMouseArea.containsMouse || curvinessMouseArea.pressed || offsetMouseArea.containsMouse || offsetMouseArea.pressed
      overlayColor: contentItem.fadeOverlayColor
      pxPerTick: root.ruler.pxPerTick
      region: root.region
      visible: fadeOverlayControl.fadePx > 0
      width: fadeOverlayControl.fadePx
      x: 0
      y: 0

      MouseArea {
        id: curvinessMouseArea

        property real startCurviness: 0
        property real startY: 0

        acceptedButtons: Qt.LeftButton
        height: parent.height / 2
        hoverEnabled: true
        preventStealing: true
        visible: fadeOverlayControl.fadePx > 0
        width: parent.width
        z: 1

        onEntered: () => {
          if (fadeOverlayControl.fadeType === FadeOverlayCanvas.FadeIn)
            CursorManager.setFadeInCursor();
          else
            CursorManager.setFadeOutCursor();
        }
        onExited: () => {
          if (!curvinessMouseArea.pressed)
            CursorManager.unsetCursor();
        }
        onPositionChanged: mouse => {
          if (!pressed)
            return;
          mouse.accepted = true;
          const dy = startY - mouse.y;
          fadeOverlayControl.curveOptsObj.curviness = Math.max(-1.0, Math.min(1.0, startCurviness + dy * 0.008));
        }
        onPressed: mouse => {
          mouse.accepted = true;
          startY = mouse.y;
          root.currentAction = fadeOverlayControl.curvinessAction;
          startCurviness = fadeOverlayControl.curveOptsObj.curviness;
        }
        onReleased: mouse => {
          mouse.accepted = true;
          const op = root.fadePropertyOperator;
          if (op !== null) {
            const finalVal = fadeOverlayControl.curveOptsObj.curviness;
            fadeOverlayControl.curveOptsObj.curviness = startCurviness;
            op.currentObject = fadeOverlayControl.curveOptsObj;
            op.setValue("curviness", finalVal);
          }
          root.currentAction = Arranger.None;
          CursorManager.unsetCursor();
        }
      }
    }

    Item {
      id: offsetStrip

      height: 12
      width: parent.width
      z: 2

      MouseArea {
        id: offsetMouseArea

        property real startContentX: 0
        property real startFadeTicks: 0

        acceptedButtons: Qt.LeftButton
        anchors.fill: parent
        cursorShape: Qt.SizeHorCursor
        enabled: fadeOverlayControl.offsetObj != null
        hoverEnabled: true
        preventStealing: true

        onPositionChanged: mouse => {
          if (!pressed)
            return;
          mouse.accepted = true;
          const currentContentX = offsetMouseArea.mapToItem(contentItem, mouse.x, 0).x;
          const dx = currentContentX - startContentX;
          const direction = fadeOverlayControl.fadeType === FadeOverlayCanvas.FadeIn ? 1 : -1;
          fadeOverlayControl.offsetObj.ticks = Math.max(0, startFadeTicks + direction * dx / root.ruler.pxPerTick);
        }
        onPressed: mouse => {
          mouse.accepted = true;
          startContentX = offsetMouseArea.mapToItem(contentItem, mouse.x, 0).x;
          root.currentAction = fadeOverlayControl.offsetAction;
          startFadeTicks = fadeOverlayControl.offsetObj.ticks;
        }
        onReleased: mouse => {
          mouse.accepted = true;
          const op = root.fadePropertyOperator;
          if (op !== null) {
            const finalVal = fadeOverlayControl.offsetObj.ticks;
            fadeOverlayControl.offsetObj.ticks = startFadeTicks;
            op.currentObject = fadeOverlayControl.offsetObj;
            op.setValue("ticks", finalVal);
          }
          root.currentAction = Arranger.None;
        }
      }
    }
  }
}
