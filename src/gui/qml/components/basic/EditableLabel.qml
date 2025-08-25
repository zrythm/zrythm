// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Control {
  id: root

  property bool currentlyEditing: false
  property color editBackgroundColor: palette.base
  property color editBorderColor: palette.accent
  property string text: ""
  property color textColor: palette.text

  implicitHeight: 24
  implicitWidth: 120

  signal accepted(text: string)

  Rectangle {
    id: background

    anchors.fill: parent
    // border.color: root.currentlyEditing ? root.editBorderColor : "transparent"
    // border.width: root.currentlyEditing ? 1 : 0
    color: root.currentlyEditing ? root.editBackgroundColor : "transparent"
    radius: 2

    Behavior on border.color {
      ColorAnimation {
        duration: 200
      }
    }
    Behavior on color {
      ColorAnimation {
        duration: 200
      }
    }
  }

  Label {
    id: displayText

    color: root.textColor
    elide: Text.ElideRight
    text: root.text
    verticalAlignment: Text.AlignVCenter
    visible: !root.currentlyEditing

    anchors {
      fill: parent
      leftMargin: 4
      rightMargin: 4
    }

    MouseArea {
      anchors.fill: parent

      onDoubleClicked: {
        root.currentlyEditing = true;
        editField.forceActiveFocus();
        editField.selectAll();
      }
    }
  }

  TextField {
    id: editField

    text: root.text
    visible: root.currentlyEditing

    Keys.onEscapePressed: {
      text = root.text;
      root.currentlyEditing = false;
    }
    onAccepted: {
      if (text !== root.text) {
        root.accepted(text);
      }
      root.currentlyEditing = false;
    }
    onFocusChanged: {
      if (!focus) {
        root.currentlyEditing = false;
      }
    }

    anchors {
      fill: parent
      leftMargin: 2
      rightMargin: 2
    }
  }
}
