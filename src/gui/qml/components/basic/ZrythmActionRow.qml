// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle 1.0

Item {
  id: control

  default property alias content: suffixLayout.data
  property string subtitle: "bbbb"
  property string title: "aaaa"

  Layout.fillWidth: true
  implicitHeight: layout.implicitHeight

  RowLayout {
    id: layout

    anchors.fill: parent
    spacing: 12

    ColumnLayout {
      spacing: 2

      Label {
        id: titleLabel

        Layout.fillWidth: true
        elide: Text.ElideRight
        font.bold: true
        text: control.title
      }

      Label {
        id: subtitleLabel

        Layout.fillWidth: true
        elide: Text.ElideRight
        font.pixelSize: titleLabel.font.pixelSize * 0.9
        opacity: 0.7
        text: control.subtitle
        visible: text !== ""
      }
    }

    RowLayout {
      id: suffixLayout

      Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
      Layout.fillWidth: true
      spacing: 8
    }
  }
}
