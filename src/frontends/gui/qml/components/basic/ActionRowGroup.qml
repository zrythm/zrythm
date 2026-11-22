// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle

// Grouped rows in a boxed card, with a bold title, an optional subtitle
// and optional trailing actions in the header.
ColumnLayout {
  id: root

  // Rows displayed inside the card.
  default property alias content: rowsColumn.children
  property string subtitle: ""
  // Actions shown as trailing icon buttons in the header.
  property list<Action> suffixActions
  property string title: ""

  spacing: ZrythmTheme.buttonPadding

  RowLayout {
    Layout.fillWidth: true
    spacing: ZrythmTheme.buttonPadding

    ColumnLayout {
      id: headerTextColumn

      Layout.fillWidth: true
      spacing: 0

      Label {
        Layout.fillWidth: true
        color: root.palette.text
        font: ZrythmTheme.semiBoldTextFont
        text: root.title
        visible: root.title !== ""
        wrapMode: Text.WordWrap
      }

      Label {
        Layout.fillWidth: true
        color: root.palette.text
        font: ZrythmTheme.smallTextFont
        opacity: 0.55
        text: root.subtitle
        visible: root.subtitle !== ""
        wrapMode: Text.WordWrap
      }
    }

    RowLayout {
      Repeater {
        model: root.suffixActions

        delegate: ToolButton {
          id: suffixButton

          required property Action modelData

          Accessible.name: suffixButton.modelData?.text ?? ""
          action: suffixButton.modelData
          display: AbstractButton.IconOnly

          ToolTip {
            text: suffixButton.modelData?.text ?? ""
          }
        }
      }
    }
  }

  Rectangle {
    id: card

    Layout.fillHeight: true
    Layout.fillWidth: true
    color: ZrythmTheme.cardColor
    implicitHeight: rowsColumn.implicitHeight
    radius: ZrythmTheme.buttonRadius

    ColumnLayout {
      id: rowsColumn

      anchors.fill: parent
      spacing: 0
    }
  }
}
