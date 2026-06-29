// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm

Control {
  id: root

  required property ClipPlaybackService clipPlaybackService
  required property ClipSlot clipSlot
  required property ArrangerObjectCreator objectCreator
  readonly property ArrangerObject clipObject: clipSlot.clipObject
  required property Scene scene
  required property Track track

  signal filesDropped(var filePaths)
  signal slotLaunched

  hoverEnabled: true

  Rectangle {
    anchors.fill: parent
    clip: true
    color: {
      let c = root.clipSlot.clipObject ? root.track.color : palette.button;
      return ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.activeFocus, tapHandler.pressed);
    }
    radius: ZrythmTheme.textFieldRadius

    Loader {
      active: root.clipSlot.clipObject !== null && root.clipSlot.clipObject.type === ArrangerObject.MidiClip
      anchors.fill: parent
      visible: active

      sourceComponent: MidiClipContent {
        contentHeight: parent.height
        contentWidth: parent.width
        midiClip: root.clipSlot.clipObject as MidiClip
      }
    }

    Loader {
      active: root.clipSlot.clipObject !== null && root.clipSlot.clipObject.type === ArrangerObject.AudioClip
      anchors.fill: parent
      visible: active

      sourceComponent: AudioClipContent {
        contentHeight: parent.height
        contentWidth: parent.width
        audioClip: root.clipSlot.clipObject as AudioClip
      }
    }

    ClipLaunchButton {
      id: playButton

      clipPlaybackService: root.clipPlaybackService
      clipSlot: root.clipSlot
      enabled: root.clipObject !== null
      track: root.track

      anchors {
        left: parent.left
        top: parent.top
      }
    }

    // Playback position indicator
    Rectangle {
      id: playbackPositionIndicator

      color: palette.text
      opacity: 0.6
      visible: root.clipSlot.state === ClipSlot.Playing || root.clipSlot.state === ClipSlot.PlayQueued
      width: 2

      anchors {
        bottom: parent.bottom
        top: parent.top
      }

      // Update position based on playback progress
      Timer {
        interval: 16 // ~60fps
        repeat: true
        running: playbackPositionIndicator.visible

        onTriggered: {
          let position = root.clipPlaybackService.getClipPlaybackPosition(root.clipSlot);
          if (position >= 0) {
            playbackPositionIndicator.x = position * root.width;
          }
        }
      }
    }

    TapHandler {
      id: tapHandler

      onDoubleTapped: {
        console.log("double tapped");
        let midiRegion = root.objectCreator.addEmptyMidiClipToClip(root.track, root.clipSlot) as MidiClip;
        root.objectCreator.addMidiNote(midiRegion, 0, 64);
        root.objectCreator.addMidiNote(midiRegion, 120, 68);
      }
    }

    DropArea {
      id: dropArea

      anchors.fill: parent

      onDropped: drop => {
        console.log("Drop on clip slot");
        let uniqueFilePaths = DragUtils.getUniqueFilePaths(drop);
        root.filesDropped(uniqueFilePaths);
      }
    }
  }
}
