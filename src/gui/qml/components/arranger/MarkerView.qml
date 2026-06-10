// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmStyle
import Zrythm

ArrangerObjectBaseView {
  id: root

  height: textMetrics.height + 2 * ZrythmTheme.buttonPadding
  width: textMetrics.width + 2 * ZrythmTheme.buttonPadding

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: ZrythmTheme.toolButtonRadius
  }

  Text {
    id: nameText

    anchors.centerIn: parent
    color: root.palette.text
    font: root.font
    padding: ZrythmTheme.buttonPadding
    text: root.arrangerObject.name.name
  }

  TextMetrics {
    id: textMetrics

    font: nameText.font
    text: nameText.text
  }
}
