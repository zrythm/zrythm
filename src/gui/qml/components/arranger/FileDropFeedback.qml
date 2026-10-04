// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick

import Zrythm

// File drop feedback: probes the dragged file once, tracks the snapped
// drop position and shows a drop line with a ghost clip preview over the
// hovered lane or track area
Item {
  id: feedback

  property bool accepted: false
  property bool active: false

  // The owning timeline, providing hit-testing helpers, snapping and the
  // ruler scale
  required property Arranger arranger
  property TrackLane dropTargetLane: null
  property Track dropTargetTrack: null
  readonly property real dropTicks: feedback.fileDropTicks
  property real durationTicks: 0
  property real fileDropTicks: 0
  required property FileImporter fileImporter
  property string fileName: ""
  property real probedDurationTicks: -1
  property string probedFilePath: ""
  property real targetHeight: 0
  property real targetY: 0

  function reset(): void {
    feedback.active = false;
    feedback.probedFilePath = "";
    feedback.probedDurationTicks = -1;
  }

  function updateForDrag(drag: DragEvent): void {
    const filePaths = DragUtils.getUniqueFilePaths(drag);
    if (filePaths.length === 0) {
      reset();
      return;
    }

    const filePath = filePaths[0];
    if (filePath !== feedback.probedFilePath) {
      feedback.probedFilePath = filePath;
      feedback.probedDurationTicks = feedback.fileImporter.getFileDurationTicks(filePath);
    }
    const fileType = feedback.fileImporter.getFileType(filePath);

    const pxPerTick = feedback.arranger.ruler.pxPerTick;
    const ticks = drag.x / pxPerTick;
    feedback.fileDropTicks = feedback.arranger.shouldSnap ? feedback.arranger.snapGrid.snapWithoutStartTicks(ticks) : ticks;
    feedback.durationTicks = Math.max(0, feedback.probedDurationTicks);
    const separatorIndex = Math.max(filePath.lastIndexOf("/"), filePath.lastIndexOf("\\"));
    feedback.fileName = filePath.substring(separatorIndex + 1);

    const timeline = feedback.arranger as Timeline;
    const track = timeline.getTrackAtY(drag.y);
    const trackLane = track ? timeline.getTrackLaneAtY(drag.y) : null;
    feedback.dropTargetTrack = track;
    feedback.dropTargetLane = trackLane;
    feedback.targetY = 0;
    feedback.targetHeight = 0;
    if (track) {
      // Map the hovered delegate's bounds into this item's coordinate
      // space, so the preview aligns with the track regardless of scrolling
      const trackItem = timeline.getTrackItemAtY(drag.y);
      if (trackItem) {
        feedback.targetY = trackItem.mapToItem(feedback, 0, 0).y;
        feedback.targetHeight = track.height;
        if (trackLane) {
          const trackLaneItem = timeline.getTrackLaneItem(trackLane, trackItem);
          if (trackLaneItem) {
            feedback.targetY = trackLaneItem.mapToItem(feedback, 0, 0).y;
            feedback.targetHeight = trackLane.height;
          }
        }
      }
    } else {
      // Below all tracks: preview where a new track would be created
      const lastTrackItem = timeline.getLastTrackItem();
      if (lastTrackItem) {
        feedback.targetY = lastTrackItem.mapToItem(feedback, 0, lastTrackItem.height).y;
        feedback.targetHeight = lastTrackItem.height;
      }
    }

    if (!track) {
      // Below all tracks: dropping creates new tracks, which accepts any
      // importable file type
      feedback.accepted = fileType !== FileImporter.FileType.Unsupported;
    } else {
      switch (fileType) {
      case FileImporter.FileType.Audio:
        feedback.accepted = track.type === Track.Audio;
        break;
      case FileImporter.FileType.Midi:
        feedback.accepted = track.type === Track.Midi || track.type === Track.Instrument;
        break;
      default:
        feedback.accepted = false;
      }
    }
    feedback.active = true;
  }

  Rectangle {
    id: dropLine

    color: feedback.accepted ? ZrythmTheme.successColor : ZrythmTheme.errorColor
    height: parent.height
    visible: feedback.active
    width: 2
    x: feedback.fileDropTicks * feedback.arranger.ruler.pxPerTick - width / 2
    z: 100
  }

  Rectangle {
    id: ghostClip

    border.color: feedback.accepted ? ZrythmTheme.successColor : ZrythmTheme.errorColor
    border.width: 1
    color: Qt.alpha(feedback.accepted ? ZrythmTheme.successColor : ZrythmTheme.errorColor, 0.15)
    height: feedback.targetHeight
    radius: ZrythmTheme.buttonRadius
    visible: feedback.active && feedback.targetHeight > 0
    width: Math.max(feedback.durationTicks * feedback.arranger.ruler.pxPerTick, 4)
    x: feedback.fileDropTicks * feedback.arranger.ruler.pxPerTick
    y: feedback.targetY
    z: 100

    Text {
      anchors.left: parent.left
      anchors.leftMargin: ZrythmTheme.buttonPadding
      anchors.right: parent.right
      anchors.rightMargin: ZrythmTheme.buttonPadding
      anchors.verticalCenter: parent.verticalCenter
      color: feedback.accepted ? ZrythmTheme.successColor : ZrythmTheme.errorColor
      elide: Text.ElideRight
      font: ZrythmTheme.arrangerObjectTextFont
      text: feedback.fileName
    }
  }
}
