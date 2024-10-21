// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.ComboBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)
    leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    hoverEnabled: true
    font: Style.buttonTextFont
    opacity: Style.getOpacity(control.enabled, control.Window.active)
    palette: Style.colorPalette

    delegate: ItemDelegate {
        required property var model
        required property int index

        width: ListView.view.width - 2
        anchors.horizontalCenter: parent.horizontalCenter
        text: model[control.textRole]
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
    }

    indicator: ColorImage {
        readonly property real additionalPadding: 4

        x: control.mirrored ? (control.padding + additionalPadding) : control.width - width - control.padding - additionalPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 16
        height: 16
        color: control.palette.dark
        source: "qrc:/qt/qml/Zrythm/icons/lucide/chevrons-up-down.svg"
    }

    contentItem: T.TextField {
        leftPadding: !control.mirrored ? 12 : control.editable && activeFocus ? 3 : 1
        rightPadding: control.mirrored ? 12 : control.editable && activeFocus ? 3 : 1
        topPadding: 6 - control.padding
        bottomPadding: 6 - control.padding
        text: control.editable ? control.editText : control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse
        color: control.editable ? control.palette.text : control.palette.buttonText
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: Text.AlignVCenter

        background: Rectangle {
            visible: control.enabled && control.editable && !control.flat
            border.width: parent && parent.activeFocus ? 2 : 1
            border.color: parent && parent.activeFocus ? control.palette.highlight : control.palette.button
            color: control.palette.base
        }

    }

    background: Rectangle {
        implicitWidth: 140
        implicitHeight: Style.buttonHeight
        color: Style.adjustColorForHoverOrVisualFocusOrDown(control.palette.button, control.hovered, control.visualFocus, control.down)
        border.color: control.palette.highlight
        border.width: !control.editable && (control.visualFocus || control.down) ? 2 : 0
        visible: !control.flat || control.down
        radius: Style.buttonRadius

        Behavior on color {
            animation: Style.propertyAnimation
        }

        Behavior on border.width {
            animation: Style.propertyAnimation
        }

    }

    popup: T.Popup {
        y: control.height
        width: control.width
        height: Math.min(contentItem.implicitHeight, control.Window.height - topMargin - bottomMargin)
        topMargin: 4
        bottomMargin: 4
        palette: control.palette
        background: PopupBackgroundRect {}

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0
            headerPositioning: ListView.OverlayHeader
            footerPositioning: ListView.OverlayFooter

            header: Item {
                height: Style.buttonRadius
            }

            footer: Item {
                height: Style.buttonRadius
            }

            T.ScrollIndicator.vertical: ScrollIndicator {
            }

        }

        enter: Transition {
            ParallelAnimation {
                PropertyAnimation {
                    property: "y"
                    from: 0
                    to: control.height - 1
                    duration: Style.animationDuration
                    easing.type: Style.animationEasingType
                }

                PropertyAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: Style.animationDuration
                    easing.type: Style.animationEasingType
                }

            }

        }

        exit: Transition {
            PropertyAnimation {
                property: "opacity"
                to: 0
                duration: Style.animationDuration
                easing.type: Style.animationEasingType
            }

        }

    }

}
