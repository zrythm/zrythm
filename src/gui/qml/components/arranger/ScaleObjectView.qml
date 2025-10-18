// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm
import ZrythmStyle

ArrangerObjectBaseView {
  id: root

  readonly property ScaleObject scaleObject: arrangerObject as ScaleObject

  height: textMetrics.height + 2 * Style.buttonPadding
  width: textMetrics.width + 2 * Style.buttonPadding

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: Style.toolButtonRadius
  }

  Text {
    id: nameText

    color: root.palette.text
    font: root.font
    padding: Style.buttonPadding
    text: {
      // bindings
      root.scaleObject.scale.scaleType;
      root.scaleObject.scale.rootKey;
      return root.scaleObject.scale.toString();
    }
  }

  TextMetrics {
    id: textMetrics

    font: nameText.font
    text: nameText.text
  }
}
