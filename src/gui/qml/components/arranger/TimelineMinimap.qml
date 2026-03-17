// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

Item {
  id: root

  enum Action {
    None,
    ResizingL,
    ResizingR,
    StartingMoving,
    Moving
  }

  readonly property real contentWidth: ruler.contentWidth
  property int currentAction: TimelineMinimap.Action.None
  property real dragStartSelectionEnd: 0
  property real dragStartSelectionStart: 0
  property real dragStartX: 0
  required property EditorSettings editorSettings
  readonly property int resizeEdgeWidth: 8
  required property Ruler ruler
  required property real scrollViewWidth
  readonly property real scrollX: editorSettings?.x ?? 0
  property real startZoomLevel: 1.0
  required property Tracklist tracklist
  readonly property real viewportStartRatio: contentWidth > 0 ? scrollX / contentWidth : 0
  readonly property real viewportWidthRatio: contentWidth > 0 ? scrollViewWidth / contentWidth : 1

  function moveSelectionX(offsetX: real) {
    const newWX = dragStartSelectionStart + offsetX;
    const ratio = newWX / minimapBackground.width;
    let rulerPx = contentWidth * ratio;

    // Clamp to valid range
    rulerPx = Math.max(0, Math.min(rulerPx, contentWidth - scrollViewWidth));
    editorSettings.x = rulerPx;
  }

  function resizeSelectionL(offsetX: real) {
    const newL = dragStartSelectionStart + offsetX;
    const oldSelectionWidth = dragStartSelectionEnd - dragStartSelectionStart;
    const newSelectionWidth = dragStartSelectionEnd - newL;

    if (newSelectionWidth < 10)
      return;

    // Calculate zoom ratio
    const ratio = newSelectionWidth / oldSelectionWidth;
    let newZoom = startZoomLevel / ratio;

    // Clamp zoom level
    newZoom = Math.max(ruler.minZoomLevel, Math.min(newZoom, ruler.maxZoomLevel));

    if (newZoom !== editorSettings.horizontalZoomLevel) {
      editorSettings.horizontalZoomLevel = newZoom;

      // Update scroll position to maintain alignment
      const posRatio = newL / minimapBackground.width;
      const rulerPx = contentWidth * posRatio;
      editorSettings.x = Math.max(0, rulerPx);
    }
  }

  function resizeSelectionR(offsetX: real) {
    const newR = dragStartSelectionEnd + offsetX;
    const oldSelectionWidth = dragStartSelectionEnd - dragStartSelectionStart;
    const newSelectionWidth = newR - dragStartSelectionStart;

    if (newSelectionWidth < 10)
      return;

    // Calculate zoom ratio
    const ratio = newSelectionWidth / oldSelectionWidth;
    let newZoom = startZoomLevel / ratio;

    // Clamp zoom level
    newZoom = Math.max(ruler.minZoomLevel, Math.min(newZoom, ruler.maxZoomLevel));

    if (newZoom !== editorSettings.horizontalZoomLevel) {
      editorSettings.horizontalZoomLevel = newZoom;

      // Keep left edge stable
      const posRatio = dragStartSelectionStart / minimapBackground.width;
      const rulerPx = contentWidth * posRatio;
      editorSettings.x = Math.max(0, rulerPx);
    }
  }

  implicitHeight: 32
  implicitWidth: 64

  Rectangle {
    anchors.fill: parent
    color: palette.window
  }

  Canvas {
    id: minimapBackground

    property real totalTicks: root.ruler.maxBars * root.ruler.ticksPerSixteenth * 16

    anchors.fill: parent
    layer.enabled: true

    onPaint: {
      const ctx = getContext("2d");
      ctx.clearRect(0, 0, width, height);

      if (!root.tracklist || !root.tracklist.collection)
        return;

      const trackModel = root.tracklist.collection;
      const trackCount = trackModel.rowCount();

      if (trackCount === 0)
        return;

      // Calculate total visible track height
      let totalTrackHeight = 0;
      const tracks = [];
      for (let i = 0; i < trackCount; i++) {
        const trackIndex = trackModel.index(i, 0);
        const track = trackModel.data(trackIndex, TrackCollection.TrackPtrRole);
        if (track && track.visible) {
          tracks.push(track);
          totalTrackHeight += track.height;
        }
      }

      if (totalTrackHeight === 0)
        return;

      // Draw regions for each track
      let currentY = 0;
      for (const track of tracks) {
        const trackHeightRatio = track.height / totalTrackHeight;
        const trackY = currentY / totalTrackHeight * height;
        const trackHeight = trackHeightRatio * height;

        // Check if track has lanes
        if (track.lanes) {
          const laneCount = track.lanes.rowCount();
          for (let l = 0; l < laneCount; l++) {
            const laneIndex = track.lanes.index(l, 0);
            const lane = track.lanes.data(laneIndex, TrackLaneList.TrackLanePtrRole);

            if (!lane)
              continue;

            // Draw MIDI regions
            if (lane.midiRegions) {
              const regionCount = lane.midiRegions.rowCount();
              for (let r = 0; r < regionCount; r++) {
                const regionIndex = lane.midiRegions.index(r, 0);
                const region = lane.midiRegions.data(regionIndex, ArrangerObjectListModel.ArrangerObjectPtrRole);

                if (region) {
                  const startX = region.position.ticks / totalTicks * width;
                  const regionWidth = Math.max(2, region.bounds.length.ticks / totalTicks * width);

                  ctx.fillStyle = Qt.alpha(track.color, 0.6);
                  ctx.fillRect(startX, trackY, regionWidth, trackHeight);
                }
              }
            }

            // Draw audio regions
            if (lane.audioRegions) {
              const regionCount = lane.audioRegions.rowCount();
              for (let r = 0; r < regionCount; r++) {
                const regionIndex = lane.audioRegions.index(r, 0);
                const region = lane.audioRegions.data(regionIndex, ArrangerObjectListModel.ArrangerObjectPtrRole);

                if (region) {
                  const startX = region.position.ticks / totalTicks * width;
                  const regionWidth = Math.max(2, region.bounds.length.ticks / totalTicks * width);

                  ctx.fillStyle = Qt.alpha(track.color, 0.6);
                  ctx.fillRect(startX, trackY, regionWidth, trackHeight);
                }
              }
            }
          }
        }

        currentY += track.height;
      }
    }

    Connections {
      function onCollectionChanged() {
        minimapBackground.requestPaint();
      }

      target: root.tracklist
    }

    Timer {
      interval: 500
      repeat: true
      running: true

      onTriggered: minimapBackground.requestPaint()
    }
  }

  Rectangle {
    id: viewportIndicator

    border.color: Qt.alpha(palette.buttonText, 0.8)
    border.width: 2
    color: Qt.alpha(palette.buttonText, 0.15)
    height: parent.height
    width: Math.max(20, root.viewportWidthRatio * parent.width)
    x: root.viewportStartRatio * parent.width
  }

  MouseArea {
    id: mouseArea

    property real startX: 0

    acceptedButtons: Qt.LeftButton
    anchors.fill: parent
    hoverEnabled: true
    preventStealing: true

    onPositionChanged: mouse => {
      const viewportX = viewportIndicator.x;
      const viewportWidth = viewportIndicator.width;

      if (!pressed) {
        // Update cursor based on position
        if (mouse.x < viewportX + root.resizeEdgeWidth && mouse.x >= viewportX - root.resizeEdgeWidth) {
          cursorShape = Qt.SizeHorCursor;
        } else if (mouse.x > viewportX + viewportWidth - root.resizeEdgeWidth && mouse.x <= viewportX + viewportWidth + root.resizeEdgeWidth) {
          cursorShape = Qt.SizeHorCursor;
        } else if (mouse.x >= viewportX && mouse.x <= viewportX + viewportWidth) {
          cursorShape = Qt.OpenHandCursor;
        } else {
          cursorShape = Qt.ArrowCursor;
        }
        return;
      }

      const offsetX = mouse.x - startX;

      if (root.currentAction === TimelineMinimap.Action.StartingMoving) {
        root.currentAction = TimelineMinimap.Action.Moving;
      }

      switch (root.currentAction) {
      case TimelineMinimap.Action.Moving:
        root.moveSelectionX(offsetX);
        break;
      case TimelineMinimap.Action.ResizingL:
        root.resizeSelectionL(offsetX);
        break;
      case TimelineMinimap.Action.ResizingR:
        root.resizeSelectionR(offsetX);
        break;
      }
    }
    onPressed: mouse => {
      startX = mouse.x;
      root.startZoomLevel = root.editorSettings.horizontalZoomLevel;

      const viewportX = viewportIndicator.x;
      const viewportWidth = viewportIndicator.width;

      if (mouse.x < viewportX + root.resizeEdgeWidth && mouse.x >= viewportX - root.resizeEdgeWidth) {
        // Left edge resize
        root.currentAction = TimelineMinimap.Action.ResizingL;
        root.dragStartSelectionStart = viewportX;
        root.dragStartSelectionEnd = viewportX + viewportWidth;
      } else if (mouse.x > viewportX + viewportWidth - root.resizeEdgeWidth && mouse.x <= viewportX + viewportWidth + root.resizeEdgeWidth) {
        // Right edge resize
        root.currentAction = TimelineMinimap.Action.ResizingR;
        root.dragStartSelectionStart = viewportX;
        root.dragStartSelectionEnd = viewportX + viewportWidth;
      } else if (mouse.x >= viewportX && mouse.x <= viewportX + viewportWidth) {
        // Moving
        root.currentAction = TimelineMinimap.Action.StartingMoving;
        root.dragStartSelectionStart = viewportX;
        root.dragStartSelectionEnd = viewportX + viewportWidth;
        cursorShape = Qt.ClosedHandCursor;
      } else {
        // Click outside - center view on click position
        root.currentAction = TimelineMinimap.Action.StartingMoving;
        const selectionWidth = viewportIndicator.width;
        root.dragStartSelectionStart = mouse.x - selectionWidth / 2;
        root.dragStartSelectionEnd = mouse.x + selectionWidth / 2;
        root.moveSelectionX(0);
      }
    }
    onReleased: {
      root.currentAction = TimelineMinimap.Action.None;
    }
    onWheel: wheel => {
      if (wheel.modifiers & Qt.ControlModifier) {
        const multiplier = wheel.angleDelta.y > 0 ? 1.3 : 1 / 1.3;
        root.editorSettings.horizontalZoomLevel = Math.min(Math.max(root.editorSettings.horizontalZoomLevel * multiplier, root.ruler.minZoomLevel), root.ruler.maxZoomLevel);
      }
    }
  }
}
