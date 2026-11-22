// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick

import Zrythm

// Plugin drop feedback: during a plugin descriptor drag, highlights the
// hovered track when it can host the plugin, the position below the last
// track where a new track with the plugin would be created, or the hovered
// track in a rejection color when it cannot host the plugin
Item {
  id: root

  property bool accepted: false
  property bool active: false

  // The owning timeline, providing drop-target geometry
  required property Arranger arranger

  property Track dropTargetTrack: null
  property real targetHeight: 0
  property real targetY: 0

  function reset(): void {
    root.active = false;
    root.dropTargetTrack = null;
  }

  function updateForDrag(drag: DragEvent): void {
    const descSrc = drag.source as DescriptorDragItem;
    if (!descSrc || !descSrc.descriptor) {
      reset();
      return;
    }

    const timeline = root.arranger as Timeline;
    const geometry = timeline.dropTargetGeometryAtY(drag.y, root);
    root.dropTargetTrack = geometry.track;
    root.targetY = geometry.y;
    root.targetHeight = geometry.height;
    root.accepted = geometry.track === null || geometry.track.canHostPlugin(descSrc.descriptor);
    root.active = true;
  }

  Rectangle {
    id: targetHighlight

    border.color: root.accepted ? ZrythmTheme.successColor : ZrythmTheme.errorColor
    border.width: 1
    color: Qt.alpha(root.accepted ? ZrythmTheme.successColor : ZrythmTheme.errorColor, 0.15)
    height: root.targetHeight
    visible: root.active && root.targetHeight > 0
    width: parent.width
    y: root.targetY
    z: 100
  }
}
