// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle

// Fader and peak level readouts on button-fill boxes; clicking the peak
// box clears the held peak reading. Shared by the mixer strips and the
// inspector's fader section.
RowLayout {
  id: root

  required property string faderText
  required property bool peakOver
  required property bool peakVisible
  required property string peakText

  signal resetPeak

  spacing: 4

  Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 18
    color: palette.button
    radius: ZrythmTheme.textFieldRadius

    Label {
      anchors.centerIn: parent
      color: palette.buttonText
      font: ZrythmTheme.smallTextFont
      text: root.faderText
    }
  }

  Rectangle {
    // The shared ToolTip template keys on the parent's `hovered`
    // property, so the box exposes its handler's hover state
    readonly property bool hovered: peakHover.hovered

    Layout.preferredHeight: 18
    Layout.preferredWidth: 40
    color: palette.button
    radius: ZrythmTheme.textFieldRadius
    visible: root.peakVisible

    Label {
      anchors.centerIn: parent
      color: root.peakOver ? ZrythmTheme.errorTextColor : palette.buttonText
      font: ZrythmTheme.smallTextFont
      text: root.peakText
    }

    HoverHandler {
      id: peakHover

      cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
      acceptedButtons: Qt.LeftButton

      onTapped: root.resetPeak()
    }

    ToolTip {
      text: qsTr("Click to reset the peak reading")
    }
  }
}
