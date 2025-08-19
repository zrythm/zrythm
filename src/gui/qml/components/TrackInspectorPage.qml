// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  required property Track track

  ExpanderBox {
    id: expander

    Layout.fillWidth: true
    title: "Section Title"

    contentItem: ColumnLayout {

      Label {
        text: root.track.name
      }

      Label {
        text: root.track.comment
      }

      Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 50
        color: root.track.color
      }
    }
  }
}
