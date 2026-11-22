// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: root

  property int clickCount: 0
  required property string heading

  color: palette.base

  ColumnLayout {
    anchors.centerIn: parent
    width: Math.min(parent.width - 32, 360)

    Label {
      Layout.fillWidth: true
      font.bold: true
      horizontalAlignment: Text.AlignHCenter
      text: root.heading
    }

    Label {
      Layout.fillWidth: true
      horizontalAlignment: Text.AlignHCenter
      text: qsTr("This content keeps its state while docking.")
      wrapMode: Text.Wrap
    }

    TextField {
      Layout.fillWidth: true
      placeholderText: qsTr("Type here, then move this panel")
    }

    Slider {
      Layout.fillWidth: true
      value: 0.35
    }

    Button {
      Layout.alignment: Qt.AlignHCenter
      text: qsTr("Clicked %1 times").arg(root.clickCount)

      onClicked: ++root.clickCount
    }
  }
}
