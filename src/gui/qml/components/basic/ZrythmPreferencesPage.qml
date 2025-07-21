// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle 1.0

ScrollView {
  id: control

  default property alias content: contentLayout.children
  property string title: ""

  Layout.fillWidth: true
  clip: true
  contentWidth: availableWidth

  ColumnLayout {
    spacing: 0
    width: control.width

    Label {
      id: pageTitle

      Layout.fillWidth: true
      font.pointSize: 16
      font.weight: Font.Bold
      padding: 16
      text: control.title
    }

    ColumnLayout {
      id: contentLayout

      Layout.fillWidth: true
      Layout.margins: 16
      spacing: 16
    }
  }
}
