// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control

    placeholderText: "Placeholder Text"
    placeholderTextColor: palette.placeholderText
    color: palette.buttonText
    states: [
        // FIXME: these are not working properly
        State {
            name: "disabled"
            when: control.enabled === false

            PropertyChanges {
                target: control
                color: palette.placeholderText
            }

            PropertyChanges {
                target: control.background
                color: palette.button
            }

        },
        State {
            name: "enabled"
            when: control.enabled === true

            PropertyChanges {
                target: control
                color: palette.text
            }

            PropertyChanges {
                target: control.background
                color: palette.base
            }

        },
        State {
            name: "focused"
            when: control.activeFocus

            PropertyChanges {
                target: control
                color: palette.text
            }

            PropertyChanges {
                target: control.background
                color: palette.highlight
            }

        },
        State {
            name: "hovered"
            when: control.hovered

            PropertyChanges {
                target: control
                color: palette.text
            }

            PropertyChanges {
                target: control.background
                color: palette.alternateBase
            }

        }
    ]

    background: Rectangle {
        id: rect

        implicitWidth: 200
        implicitHeight: 24
        color: control.enabled ? "transparent" : palette.accent
        border.color: control.enabled ? palette.accent : "transparent"
    }

}
