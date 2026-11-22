// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle

T.SearchField {
  id: control

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, searchIndicator.implicitIndicatorHeight + topPadding + bottomPadding, clearIndicator.implicitIndicatorHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  leftPadding: padding + (control.mirrored || !searchIndicator.indicator || !searchIndicator.indicator.visible ? 0 : searchIndicator.indicator.width + spacing)
  opacity: ZrythmTheme.getOpacity(control.enabled, control.Window.active)
  padding: ZrythmTheme.buttonPadding
  rightPadding: padding + (control.mirrored || !clearIndicator.indicator || !clearIndicator.indicator.visible ? 0 : clearIndicator.indicator.width + spacing)
  spacing: ZrythmTheme.buttonPadding

  background: Rectangle {
    color: control.palette.base
    implicitHeight: ZrythmTheme.buttonHeight
    implicitWidth: 200
    radius: ZrythmTheme.textFieldRadius

    Behavior on border.color {
      animation: ZrythmTheme.propertyAnimation
    }
    Behavior on border.width {
      animation: ZrythmTheme.propertyAnimation
    }

    border {
      color: control.activeFocus || control.contentItem.activeFocus ? control.palette.highlight : control.palette.mid
      width: control.activeFocus || control.contentItem.activeFocus ? 2 : 1
    }
  }
  clearIndicator.indicator: Item {
    implicitHeight: 20
    implicitWidth: 20
    visible: control.text.length > 0
    x: control.mirrored ? control.padding : control.width - width - control.padding
    y: control.topPadding + (control.availableHeight - height) / 2

    ColorImage {
      anchors.centerIn: parent
      color: control.clearIndicator.hovered ? control.palette.text : control.palette.placeholderText
      height: 12
      source: "qrc:/qt/qml/Zrythm/icons/noto-glyphs/close.svg"
      width: 12
    }
  }
  contentItem: T.TextField {
    color: control.palette.text
    font: ZrythmTheme.normalTextFont
    implicitHeight: contentHeight + topPadding + bottomPadding
    leftPadding: 2
    rightPadding: 2
    selectedTextColor: control.palette.highlightedText
    selectionColor: control.palette.highlight
    text: control.text
    verticalAlignment: TextInput.AlignVCenter
  }
  delegate: ItemDelegate {
    required property int index
    required property var model

    font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
    highlighted: control.highlightedIndex === index
    text: model[control.textRole]
    width: ListView.view.width
  }
  popup: Popup {
    bottomMargin: 6
    height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, control.Window.height - control.y - control.height - control.padding)
    padding: 2
    palette: control.palette
    topMargin: 6
    width: control.width
    y: control.height

    contentItem: ListView {
      clip: true
      currentIndex: control.highlightedIndex
      highlightMoveDuration: 0
      implicitHeight: contentHeight
      model: control.delegateModel

      ScrollIndicator.vertical: ScrollIndicator {
      }
    }
  }
  searchIndicator.indicator: Item {
    implicitHeight: 20
    implicitWidth: 20
    x: !control.mirrored ? control.padding : control.width - width - control.padding
    y: control.topPadding + (control.availableHeight - height) / 2

    ColorImage {
      anchors.centerIn: parent
      color: control.searchIndicator.hovered ? control.palette.text : control.palette.placeholderText
      height: 14
      source: "qrc:/qt/qml/Zrythm/icons/noto-glyphs/search.svg"
      width: 14
    }
  }
}
