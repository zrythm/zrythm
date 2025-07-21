// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

RowLayout {
  id: root

  default property alias content: root.children
  property alias enableDropShadow: root.layer.enabled
  property int radius: Style.textFieldRadius

  Layout.maximumWidth: implicitWidth
  Layout.preferredWidth: implicitWidth
  layer.enabled: false
  spacing: 0

  layer.effect: DropShadowEffect {
  }

  onChildrenChanged: {
    let firstChildFound = false;
    for (let i = 0; i < children.length; i++) {
      const child = children[i];
      child.Layout.fillWidth = true;
      child.layer.enabled = false;
      // Modify the existing background
      if (child.background && child.background instanceof Rectangle) {
        child.background.bottomLeftRadius = 0;
        child.background.topLeftRadius = 0;
        child.background.bottomRightRadius = 0;
        child.background.topRightRadius = 0;
        if (!firstChildFound && child.visible) {
          child.background.bottomLeftRadius = root.radius;
          child.background.topLeftRadius = root.radius;
          firstChildFound = true;
        }
      }
    }
    // do the last child too
    for (let i = children.length - 1; i >= 0; i--) {
      const child = children[i];
      if (child.background && child.background instanceof Rectangle) {
        if (child.visible) {
          child.background.bottomRightRadius = root.radius;
          child.background.topRightRadius = root.radius;
          break;
        }
      }
    }
  }
}
