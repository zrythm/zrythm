// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmArrangement
import ZrythmGui
import ZrythmStyle

Arranger {
  id: root

  readonly property AudioClip audioClip: clipEditor.clipObject as AudioClip
  required property AudioClipEditor audioEditor
  /** Index of the transform entry selected in the stack panel, or -1 when none. */
  required property int selectedTransformEntryIndex
  readonly property real clipWidth: root.audioClip ? root.audioClip.timelineLengthTicks * root.ruler.pxPerTick : 0
  readonly property real clipX: root.audioClip ? root.audioClip.position.ticks * root.ruler.pxPerTick : 0
  readonly property real fadeInPx: root.audioClip && root.audioClip.fadeRange ? root.audioClip.fadeRange.startOffset.ticks * root.ruler.pxPerTick : 0
  readonly property real fadeOutPx: root.audioClip && root.audioClip.fadeRange ? root.audioClip.fadeRange.endOffset.ticks * root.ruler.pxPerTick : 0
  // Editor waveform ink: the shared musical-content color over the
  // clip's own color where set, else the track color, so the editor
  // renders identically to in-clip waveforms (DESIGN.md "Clips")
  readonly property color waveformInk: ZrythmTheme.contentColor (
    root.audioClip && root.audioClip.color.useColor
      ? root.audioClip.color.color : TrackColors.effectiveColor(root.track, ZrythmTheme.darkMode),
      false)
  property QObjectPropertyOperator fadePropertyOperator: null
  readonly property Track track: clipEditor.track

  // A different clip being edited invalidates the selected range
  onAudioClipChanged: {
    if (root.audioEditor) {
      root.audioEditor.selectionStartTick = -1;
      root.audioEditor.selectionEndTick = -1;
    }
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

  editorSettings: audioEditor
  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: Item {
    id: contentItem

    readonly property color fadeCurveColor: Qt.alpha(palette.text, 0.5)
    readonly property color fadeOverlayColor: Qt.alpha(palette.shadow, 0.25)

    anchors.fill: parent

    // Drag-selects the time range that audio functions apply to. Sits below
    // the fade controls, and takes only the Select tool's left button.
    MouseArea {
      id: rangeSelectionMouseArea

      property real dragStartTick: 0
      property real dragEndTick: 0
      property real pressX: 0
      property bool rangeDragActive: false

      // Selection is bounded to the clip's audible content span: the first
      // leg plays [clip start, loop end) and loop iterations play
      // [loop start, loop end), so content outside this span never plays
      readonly property real firstAudibleContentTick: root.audioClip ? Math.min(root.audioClip.clipStartPosition.ticks, root.audioClip.loopStartPosition.ticks) : 0
      readonly property real lastAudibleContentTick: root.audioClip ? root.audioClip.loopEndPosition.ticks : 0

      function snappedContentTicks(ticks: real, startTicks: real): real {
        const bounded = Math.max(rangeSelectionMouseArea.firstAudibleContentTick, Math.min(ticks, rangeSelectionMouseArea.lastAudibleContentTick));
        return root.shouldSnap ? root.snapGrid.snapWithStartTicks(bounded, startTicks) : bounded;
      }

      acceptedButtons: Qt.LeftButton
      anchors.fill: parent
      enabled: root.audioClip !== null && root.tool.effectiveToolValue === ArrangerTool.Select
      preventStealing: true

      onPressed: mouse => {
        pressX = mouse.x;
        dragStartTick = snappedContentTicks(root.ruler.contentTicksAt(mouse.x), 0);
        dragEndTick = dragStartTick;
        rangeDragActive = true;
      }
      onPositionChanged: mouse => {
        if (!rangeDragActive)
          return;
        dragEndTick = snappedContentTicks(root.ruler.contentTicksAt(mouse.x), dragStartTick);
      }
      onReleased: mouse => {
        rangeDragActive = false;
        if (Math.abs(mouse.x - pressX) < Qt.styleHints.startDragDistance) {
          root.audioEditor.selectionStartTick = -1;
          root.audioEditor.selectionEndTick = -1;
        } else {
          root.audioEditor.selectionStartTick = Math.min(dragStartTick, dragEndTick);
          root.audioEditor.selectionEndTick = Math.max(dragStartTick, dragEndTick);
        }
      }
    }

    AudioClipWaveformCanvas {
      id: waveformCanvas

      audioClip: root.audioClip
      height: contentItem.height
      outlineColor: Qt.rgba (root.waveformInk.r, root.waveformInk.g, root.waveformInk.b, 200 / 255)
      tempoMap: root.tempoMap
      waveformColor: Qt.rgba (root.waveformInk.r, root.waveformInk.g, root.waveformInk.b, 80 / 255)
      width: root.clipWidth
      x: root.clipX
      y: 0
    }

    // Highlights the time range that audio functions apply to, both while
    // dragging and after the range is committed
    Rectangle {
      id: functionRangeOverlay

      readonly property double startTick: rangeSelectionMouseArea.rangeDragActive ? Math.min(rangeSelectionMouseArea.dragStartTick, rangeSelectionMouseArea.dragEndTick) : root.audioEditor.selectionStartTick
      readonly property double endTick: rangeSelectionMouseArea.rangeDragActive ? Math.max(rangeSelectionMouseArea.dragStartTick, rangeSelectionMouseArea.dragEndTick) : root.audioEditor.selectionEndTick
      readonly property var warp: root.audioClip ? root.audioClip.contentWarp : null
      readonly property double startRelativeTicks: warp ? warp.contentToTimelineTicksRelative(startTick) : startTick
      readonly property double endRelativeTicks: warp ? warp.contentToTimelineTicksRelative(endTick) : endTick

      border.color: palette.accent
      color: Qt.alpha(palette.accent, 0.12)
      height: parent.height
      visible: startTick >= 0 && endTick > startTick
      width: (endRelativeTicks - startRelativeTicks) * root.ruler.pxPerTick
      x: root.clipX + startRelativeTicks * root.ruler.pxPerTick
      y: 0
    }

    // Highlights the content range of the transform entry selected in the
    // stack panel, in the entry's kind color. Dimmed when the entry is
    // disabled. Hidden when no entry is selected.
    Rectangle {
      id: transformEntryOverlay

      readonly property var selectedEntry: root.selectedTransformEntryIndex >= 0 && root.audioClip && root.selectedTransformEntryIndex < root.audioClip.audioTransformEntries.length ? root.audioClip.audioTransformEntries[root.selectedTransformEntryIndex] : null
      readonly property double startTick: selectedEntry ? selectedEntry.startTicks : 0
      readonly property double endTick: selectedEntry ? selectedEntry.endTicks : 0
      readonly property var warp: root.audioClip ? root.audioClip.contentWarp : null
      readonly property double startRelativeTicks: warp ? warp.contentToTimelineTicksRelative(startTick) : startTick
      readonly property double endRelativeTicks: warp ? warp.contentToTimelineTicksRelative(endTick) : endTick

      border.color: selectedEntry ? AudioTransformKindColors.kindColor (selectedEntry.kindId) : "transparent"
      color: Qt.alpha (border.color, 0.15)
      height: parent.height
      opacity: selectedEntry && selectedEntry.enabled ? 1 : ZrythmTheme.disabledOpacityFactor
      visible: endTick > startTick
      width: (endRelativeTicks - startRelativeTicks) * root.ruler.pxPerTick
      x: root.clipX + startRelativeTicks * root.ruler.pxPerTick
      y: 0
    }

    FadeOverlayControl {
      curveOptsObj: root.audioClip ? root.audioClip.fadeRange.fadeInCurveOpts : null
      curvinessAction: Arranger.ResizingUpFadeIn
      fadePx: root.fadeInPx
      fadeType: FadeOverlayCanvas.FadeIn
      offsetAction: Arranger.ResizingLFade
      offsetObj: root.audioClip ? root.audioClip.fadeRange.startOffset : null
      x: root.clipX
      y: 0
    }

    FadeOverlayControl {
      curveOptsObj: root.audioClip ? root.audioClip.fadeRange.fadeOutCurveOpts : null
      curvinessAction: Arranger.ResizingUpFadeOut
      fadePx: root.fadeOutPx
      fadeType: FadeOverlayCanvas.FadeOut
      offsetAction: Arranger.ResizingRFade
      offsetObj: root.audioClip ? root.audioClip.fadeRange.endOffset : null
      x: root.clipX + root.clipWidth - root.fadeOutPx
      y: 0
    }

    // Gain line (TODO: make it operational)
    Item {
      id: gainLine

      readonly property real gainY: root.audioClip ? (1.0 - root.audioClip.gain / 2.0) * height : 0

      height: parent.height
      visible: root.audioClip != null
      width: root.clipWidth
      x: root.clipX
      y: 0

      Rectangle {
        color: palette.accent
        height: 2
        width: parent.width
        y: gainLine.gainY

        Label {
          color: palette.text
          font.pixelSize: 10
          text: (20 * Math.log10(root.audioClip ? root.audioClip.gain : 1.0)).toFixed(1) + " dB"
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
    required property Position offsetObj

    height: contentItem.height
    width: Math.max(fadePx, 20)

    FadeOverlayCanvas {
      audioClip: root.audioClip
      curveColor: contentItem.fadeCurveColor
      fadeType: fadeOverlayControl.fadeType
      height: parent.height
      hovered: curvinessMouseArea.containsMouse || curvinessMouseArea.pressed || offsetMouseArea.containsMouse || offsetMouseArea.pressed
      overlayColor: contentItem.fadeOverlayColor
      pxPerTick: root.ruler.pxPerTick
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
