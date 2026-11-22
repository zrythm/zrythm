// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

// Groups buttons into a pill: shared corners are flattened and only the
// outer corners of the first and last visible child are rounded.
// Horizontal by default; set `vertical` to stack the buttons top to
// bottom.
GridLayout {
  id: root

  default property alias content: root.children
  property alias enableDropShadow: root.layer.enabled
  property int radius: ZrythmTheme.textFieldRadius
  property real spacing: 0
  property bool vertical: false

  Layout.maximumWidth: root.implicitWidth
  Layout.preferredWidth: root.implicitWidth
  columnSpacing: root.spacing
  columns: root.vertical ? 1 : -1
  layer.enabled: false
  rowSpacing: root.spacing
  rows: root.vertical ? -1 : 1

  layer.effect: DropShadowEffect {
  }

  // One per direct child; re-runs the corner pass when a child's
  // visibility flips after construction (e.g. a record chip appearing
  // on a track that gains a recording parameter)
  Component {
    id: visibleWatcher

    Connections {
      id: watcher

      property Item watched

      target: watcher.watched

      function onVisibleChanged(): void {
        root.applyPillCorners();
      }
    }
  }

  property var watchers: []

  Component.onCompleted: applyPillCorners()

  onChildrenChanged: {
    for (const watcher of root.watchers)
      watcher.destroy();
    root.watchers = [];
    for (let i = 0; i < children.length; i++) {
      const child = children[i];
      child.Layout.fillWidth = true;
      child.layer.enabled = false;
      root.watchers.push(visibleWatcher.createObject(root, {
          "watched": child
        }));
    }
    applyPillCorners();
  }

  // Flattens the shared corners and rounds the pill's outer corners on
  // the backgrounds of the first and last visible children
  function applyPillCorners(): void {
    let firstChildFound = false;
    for (let i = 0; i < children.length; i++) {
      const child = children[i];
      const background = child.background;
      if (!(background instanceof Rectangle))
        continue;
      background.bottomLeftRadius = 0;
      background.bottomRightRadius = 0;
      background.topLeftRadius = 0;
      background.topRightRadius = 0;
      if (!firstChildFound && child.visible) {
        if (root.vertical) {
          background.topLeftRadius = root.radius;
          background.topRightRadius = root.radius;
        } else {
          background.topLeftRadius = root.radius;
          background.bottomLeftRadius = root.radius;
        }
        firstChildFound = true;
      }
    }
    for (let i = children.length - 1; i >= 0; i--) {
      const child = children[i];
      const background = child.background;
      if (!(background instanceof Rectangle) || !child.visible)
        continue;
      if (root.vertical) {
        background.bottomLeftRadius = root.radius;
        background.bottomRightRadius = root.radius;
      } else {
        background.topRightRadius = root.radius;
        background.bottomRightRadius = root.radius;
      }
      break;
    }
  }
}
