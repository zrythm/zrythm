// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic

ToolButton {
    id: root

    implicitHeight: 24

    property real backgroundRadius: 6

    text: "Tool Button"
    states: [
        State {
            name: "pressed"
            when: root.down

            PropertyChanges {
                target: root.contentItem
                color: palette.accent
            }

            PropertyChanges {
                target: root.background
                visible: true
                color: Qt.darker(palette.button, 1.3)
            }

        },
        State {
            name: "checked"
            when: root.enabled && (root.checked || root.highlighted)

            PropertyChanges {
                target: root.background
                visible: true
                color: Qt.darker(palette.button, 1.3)
            }

        },
        State {
            name: "hovered"
            when: root.enabled && root.hovered

            PropertyChanges {
                target: root.background
                visible: true
                color: Qt.darker(palette.button, 1.1)
            }

        }
    ]

    contentItem: Text {
        id: buttonText
        text: root.text
        font {
            family: root.font.family
            pointSize: 10
        }
        opacity: root.enabled ? 1 : 0.3
        color: palette.buttonText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        id: backgroundRect
        color: palette.button
        opacity: root.enabled ? 1 : 0.3
        visible: false
        radius: root.backgroundRadius
    }

}
