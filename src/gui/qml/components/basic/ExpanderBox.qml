// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick.Layouts
import QtQuick
import QtQuick.Controls
import ZrythmStyle

ColumnLayout {
  id: root

  property var contentItem
  property string title: "Expander"
  property bool vertical: false

  spacing: 0

  Button {
    id: expandButton

    Layout.fillHeight: root.vertical
    Layout.fillWidth: !root.vertical
    checkable: true
    checked: true
    text: root.title
  }

  Frame {
    id: frame

    Layout.fillHeight: root.vertical
    Layout.fillWidth: !root.vertical
    contentItem: root.contentItem
    visible: expandButton.checked

    Behavior on Layout.preferredHeight {
      animation: Style.propertyAnimation
    }
  }

  Binding {
    property: "Layout.preferredHeight"
    target: frame
    value: 0
    when: !expandButton.checked
  }
}
