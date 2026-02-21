// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls

Item {
  id: root

  property alias font: rotatedText.font
  property int rotation: 270
  property string text: "Text"

  implicitHeight: textMetrics.width
  implicitWidth: textMetrics.height

  TextMetrics {
    id: textMetrics

    font: rotatedText.font
    text: rotatedText.text
  }

  Label {
    id: rotatedText

    anchors.fill: parent
    horizontalAlignment: Text.AlignHCenter
    rotation: root.rotation
    text: root.text
    verticalAlignment: Text.AlignVCenter
  }
}
