// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle

T.TreeViewDelegate {
  id: control

  readonly property real __contentIndent: !isTreeNode ? 0 : (depth * indentation) + (indicator ? indicator.width + spacing : 0)
  required property var model
  required property int row

  font: Style.semiBoldTextFont
  highlighted: control.selected || control.current || ((control.treeView.selectionBehavior === TableView.SelectRows || control.treeView.selectionBehavior === TableView.SelectionDisabled) && control.row === control.treeView.currentRow)
  implicitHeight: Math.max(indicator ? indicator.height : 0, implicitContentHeight) * 1.25
  implicitWidth: leftMargin + __contentIndent + implicitContentWidth + rightPadding + rightMargin
  indentation: indicator ? indicator.width : 12
  leftMargin: 4
  leftPadding: !mirrored ? leftMargin + __contentIndent : width - leftMargin - __contentIndent - implicitContentWidth
  rightMargin: 4
  spacing: 4
  topPadding: contentItem ? (height - contentItem.implicitHeight) / 2 : 0

  // The edit delegate is a separate component, and doesn't need
  // to follow the same strict rules that are applied to a control.
  TableView.editDelegate: FocusScope {
    readonly property int __role: {
      let model = control.treeView.model;
      let index = control.treeView.index(row, column);
      let editText = model.data(index, Qt.EditRole);
      return editText !== undefined ? Qt.EditRole : Qt.DisplayRole;
    }

    height: parent.height
    width: parent.width

    Component.onCompleted: textField.selectAll()
    TableView.onCommit: {
      let index = TableView.view.index(row, column);
      TableView.view.model.setData(index, textField.text, __role);
    }

    TextField {
      id: textField

      focus: true
      text: control.treeView.model.data(control.treeView.index(row, column), __role)
      width: control.contentItem.width
      x: control.contentItem.x
      y: (parent.height - height) / 2
    }
  }
  background: Rectangle {
    readonly property color baseColor: control.highlighted ? control.palette.highlight : control.palette.button
    readonly property color colorAdjustedForHoverOrFocusOrDown: Style.adjustColorForHoverOrVisualFocusOrDown(baseColor, hoverHandler.hovered && !control.highlighted, control.visualFocus, control.down)

    border.width: Qt.styleHints.accessibility.contrastPreference !== Qt.HighContrast ? 0 : control.current ? 2 : 1
    color: colorAdjustedForHoverOrFocusOrDown
    implicitHeight: Style.buttonHeight
    implicitWidth: 100
    visible: control.down || control.highlighted || control.visualFocus || hoverHandler.hovered

    Behavior on color {
      animation: Style.propertyAnimation
    }
  }
  contentItem: Label {
    clip: false
    color: control.highlighted ? control.palette.highlightedText : control.palette.buttonText
    elide: Text.ElideRight
    text: control.model.display
    visible: !control.editing
  }
  indicator: Item {
    // Create an area that is big enough for the user to
    // click on, since the image is a bit small.
    readonly property real __indicatorIndent: control.leftMargin + (control.depth * control.indentation)

    implicitHeight: Style.buttonHeight
    implicitWidth: 20
    x: !control.mirrored ? __indicatorIndent : control.width - __indicatorIndent - width
    y: (control.height - height) / 2

    ColorImage {
      color: control.palette.windowText
      defaultColor: "#353637"
      height: 12
      rotation: control.expanded ? 90 : (control.mirrored ? 180 : 0)
      source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"
      width: 12
      x: (parent.width - width) / 2
      y: (parent.height - height) / 2
    }
  }

  HoverHandler {
    id: hoverHandler

  }
}
