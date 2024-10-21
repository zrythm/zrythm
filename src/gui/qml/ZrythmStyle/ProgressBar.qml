// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.ProgressBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    opacity: Style.getOpacity(control.enabled, control.Window.active)

    contentItem: Item {
        implicitWidth: 200
        implicitHeight: 6

        // Progress indicator for determinate state.
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 4
            color: palette.highlight
            visible: !control.indeterminate
        }

        // Scrolling animation for indeterminate state.
        Item {
            anchors.fill: parent
            visible: control.indeterminate
            clip: true

            Row {
                anchors.fill: parent
                spacing: 0

                Repeater {
                    model: 5

                    Rectangle {
                        id: rectangle

                        width: control.width / 3
                        height: control.height
                        radius: 4
                        color: control.palette.highlight

                        NumberAnimation on x {
                            from: -rectangle.width
                            to: control.width
                            duration: 1500
                            loops: Animation.Infinite
                            easing.type: Easing.InOutQuad
                        }

                    }

                }

            }

        }

    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 6
        // y: (control.height - height) / 2
        // height: 6
        radius: 3
        color: control.palette.text
    }

}
