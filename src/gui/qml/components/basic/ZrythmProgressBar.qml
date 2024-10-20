// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound
import QtQuick
//import QtQuick.Controls.Basic
import ZrythmStyle 1.0

ProgressBar {
    id: control

    value: 0.5
    padding: 2

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
        color: palette.base
        radius: 3
    }

}
