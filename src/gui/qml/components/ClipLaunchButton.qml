// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm

ToolButton {
  id: root

  required property ClipPlaybackService clipPlaybackService
  required property ClipSlot clipSlot
  readonly property bool shouldAnimate: root.clipSlot.state === ClipSlot.StopQueued || root.clipSlot.state === ClipSlot.PlayQueued
  property bool showQuantizationMenu: true
  required property Track track

  function performDefaultAction() {
    switch (root.clipSlot.state) {
    case ClipSlot.Stopped:
    case ClipSlot.StopQueued:
      root.clipPlaybackService.launchClip(root.track, root.clipSlot);
      break;
    case ClipSlot.Playing:
    case ClipSlot.PlayQueued:
      root.clipPlaybackService.stopClip(root.track, root.clipSlot);
      break;
    }
  }

  icon.source: {
    switch (root.clipSlot.state) {
    case ClipSlot.Stopped:
    case ClipSlot.StopQueued:
      return ResourceManager.getIconUrl("gnome-icon-library", "media-playback-start-symbolic.svg");
    case ClipSlot.Playing:
    case ClipSlot.PlayQueued:
      return ResourceManager.getIconUrl("gnome-icon-library", "media-playback-stop-symbolic.svg");
    }
  }

  PropertyAnimation on opacity {
    id: flashAnimation

    duration: 600
    easing.type: Easing.InOutQuad
    from: 1.0
    loops: Animation.Infinite
    running: false
    to: 0.3
  }

  onClicked: {
    performDefaultAction();
  }

  MouseArea {
    anchors.fill: parent
    acceptedButtons: Qt.RightButton

    onClicked: {
      if (root.showQuantizationMenu && (root.clipSlot.state === ClipSlot.Stopped || root.clipSlot.state === ClipSlot.StopQueued)) {
        quantizeMenu.popup();
      }
    }
  }
  onShouldAnimateChanged: {
    if (shouldAnimate) {
      flashAnimation.start();
    } else {
      flashAnimation.stop();
      opacity = 1.0;
    }
  }

  ToolTip {
    text: {
      switch (root.clipSlot.state) {
      case ClipSlot.Stopped:
      case ClipSlot.StopQueued:
        return qsTr("Start Playback") + (root.showQuantizationMenu ? qsTr(" (Right-click for quantization options)") : "");
      case ClipSlot.Playing:
      case ClipSlot.PlayQueued:
        return qsTr("Stop Playback");
      }
    }
  }

  Menu {
    id: quantizeMenu

    title: qsTr("Launch Quantization")

    MenuItem {
      text: qsTr("Launch Immediately")

      onClicked: root.clipPlaybackService.launchClip(root.track, root.clipSlot, ClipSlot.ClipQuantizeOption.Immediate)
    }

    MenuItem {
      text: qsTr("Launch on Next Bar")

      onClicked: root.clipPlaybackService.launchClip(root.track, root.clipSlot, ClipSlot.ClipQuantizeOption.NextBar)
    }

    MenuItem {
      text: qsTr("Launch on Next Beat")

      onClicked: root.clipPlaybackService.launchClip(root.track, root.clipSlot, ClipSlot.ClipQuantizeOption.NextBeat)
    }
  }
}
