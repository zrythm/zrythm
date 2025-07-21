// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ArrangerObjectBaseView {
  id: root

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
      arrangerObject.scale.scaleType;
      arrangerObject.scale.rootKey;
      return arrangerObject.scale.toString();
    }
  }

  TextMetrics {
    id: textMetrics

    font: nameText.font
    text: nameText.text
  }
}
