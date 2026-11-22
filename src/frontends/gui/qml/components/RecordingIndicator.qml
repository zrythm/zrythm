// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle

// Record-red "● REC" status item, shown while recording and blinking
// with the same 1 s cycle as the transport record button.
Row {
  id: root

  required property bool recording

  spacing: 4
  visible: root.recording

  Rectangle {
    anchors.verticalCenter: parent.verticalCenter
    color: ZrythmTheme.recordColor
    height: 8
    radius: 4
    width: 8
  }

  Label {
    color: ZrythmTheme.recordTextColor
    font: ZrythmTheme.xSmallTextFont
    text: qsTr("REC")
  }

  SequentialAnimation on opacity {
    running: root.recording
    loops: Animation.Infinite

    NumberAnimation {
      duration: 500
      easing.type: Easing.InOutQuad
      to: 0.25
    }
    NumberAnimation {
      duration: 500
      easing.type: Easing.InOutQuad
      to: 1
    }

    onStopped: root.opacity = 1
  }
}
