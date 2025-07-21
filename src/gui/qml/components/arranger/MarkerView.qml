// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmArrangement 1.0
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

    anchors.centerIn: parent
    color: root.palette.text
    font: root.font
    padding: Style.buttonPadding
    text: arrangerObject.name.name
  }

  TextMetrics {
    id: textMetrics

    font: nameText.font
    text: nameText.text
  }
}
