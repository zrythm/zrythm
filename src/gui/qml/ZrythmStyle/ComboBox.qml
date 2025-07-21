// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.ComboBox {
  id: control

  font: Style.buttonTextFont
  hoverEnabled: true
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
  opacity: Style.getOpacity(control.enabled, control.Window.active)
  rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)

  background: Rectangle {
    border.color: control.palette.highlight
    border.width: !control.editable && (control.visualFocus || control.down) ? 2 : 0
    color: Style.adjustColorForHoverOrVisualFocusOrDown(control.palette.button, control.hovered, control.visualFocus, control.down)
    implicitHeight: Style.buttonHeight
    implicitWidth: 140
    radius: Style.buttonRadius
    visible: !control.flat || control.down

    Behavior on border.width {
      animation: Style.propertyAnimation
    }
    Behavior on color {
      animation: Style.propertyAnimation
    }
  }
  contentItem: T.TextField {
    autoScroll: control.editable
    bottomPadding: 6 - control.padding
    color: control.editable ? control.palette.text : control.palette.buttonText
    enabled: control.editable
    inputMethodHints: control.inputMethodHints
    leftPadding: !control.mirrored ? 12 : control.editable && activeFocus ? 3 : 1
    readOnly: control.down
    rightPadding: control.mirrored ? 12 : control.editable && activeFocus ? 3 : 1
    selectByMouse: control.selectTextByMouse
    selectedTextColor: control.palette.highlightedText
    selectionColor: control.palette.highlight
    text: control.editable ? control.editText : control.displayText
    topPadding: 6 - control.padding
    validator: control.validator
    verticalAlignment: Text.AlignVCenter

    background: Rectangle {
      border.color: parent && parent.activeFocus ? control.palette.highlight : control.palette.button
      border.width: parent && parent.activeFocus ? 2 : 1
      color: control.palette.base
      visible: control.enabled && control.editable && !control.flat
    }
  }
  delegate: ItemDelegate {
    required property int index
    required property var model

    anchors.horizontalCenter: parent.horizontalCenter
    highlighted: control.highlightedIndex === index
    hoverEnabled: control.hoverEnabled
    text: model[control.textRole]
    width: ListView.view.width - 2
  }
  indicator: ColorImage {
    readonly property real additionalPadding: 4

    color: control.palette.dark
    height: 16
    source: "qrc:/qt/qml/Zrythm/icons/lucide/chevrons-up-down.svg"
    width: 16
    x: control.mirrored ? (control.padding + additionalPadding) : control.width - width - control.padding - additionalPadding
    y: control.topPadding + (control.availableHeight - height) / 2
  }
  popup: T.Popup {
    bottomMargin: 4
    height: Math.min(contentItem.implicitHeight, control.Window.height - topMargin - bottomMargin)
    palette: control.palette
    topMargin: 4
    width: control.width
    y: control.height

    background: PopupBackgroundRect {
    }
    contentItem: ListView {
      clip: true
      currentIndex: control.highlightedIndex
      footerPositioning: ListView.OverlayFooter
      headerPositioning: ListView.OverlayHeader
      highlightMoveDuration: 0
      implicitHeight: contentHeight
      model: control.delegateModel

      T.ScrollIndicator.vertical: ScrollIndicator {
      }
      footer: Item {
        height: Style.buttonRadius
      }
      header: Item {
        height: Style.buttonRadius
      }
    }
    enter: Transition {
      ParallelAnimation {
        PropertyAnimation {
          duration: Style.animationDuration
          easing.type: Style.animationEasingType
          from: control.height / 2
          property: "y"
          to: control.height - 1
        }

        PropertyAnimation {
          duration: Style.animationDuration
          easing.type: Style.animationEasingType
          from: 0
          property: "opacity"
          to: 1
        }
      }
    }
    exit: Transition {
      PropertyAnimation {
        duration: Style.animationDuration
        easing.type: Style.animationEasingType
        property: "opacity"
        to: 0
      }
    }
  }
}
