// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

RowLayout {
  id: root

  property alias text: label.text

  Label {
    id: label

    Layout.fillHeight: true
    Layout.fillWidth: true
    horizontalAlignment: Text.AlignRight
    verticalAlignment: Text.AlignVCenter
  }
}
