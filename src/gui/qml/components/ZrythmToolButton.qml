// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts

ToolButton {
    id: root

    property real backgroundRadius: 6
    property string iconSource: "" // Qt.resolvedUrl("../icons/zrythm-dark/edit-undo.svg")
    property color buttonTextColor: palette.buttonText
    property color buttonCheckedColor: palette.accent

    padding: 4
    font.pointSize: 10
    implicitHeight: 24
    text: ""
    states: [
        State {
            name: "pressed"
            when: root.down

            PropertyChanges {
                target: root
                buttonTextColor: palette.accent
            }

            PropertyChanges {
                target: root.background
                visible: true
                color: root.checkable ? root.buttonCheckedColor : Qt.darker(palette.button, 1.3)
            }

        },
        State {
            name: "checked"
            when: root.enabled && (root.checked || root.highlighted)

            PropertyChanges {
                target: root.background
                visible: true
                color: root.buttonCheckedColor
                opacity: 0.8
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

    Component {
        id: buttonTextComponent

        Text {
            text: root.text
            opacity: root.enabled ? 1 : 0.3
            color: root.buttonTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            font {
                family: root.font.family
                pointSize: root.font.pointSize
            }

        }

    }

    Component {
        id: buttonIconComponent

        Image {
            id: buttonImage

            readonly property int iconSize: root.availableHeight

            source: root.iconSource
            fillMode: Image.PreserveAspectFit
            sourceSize.width: iconSize
            width: iconSize
            opacity: root.enabled ? 1 : 0.3
        }

    }

    contentItem: Loader {
        sourceComponent: root.text === "" ? iconOnlyComponent : (root.iconSource === "" ? textOnlyComponent : iconTextComponent)

        Component {
            id: iconTextComponent

            RowLayout {
                spacing: 4
                anchors.centerIn: parent
                anchors.fill: parent

                Loader {
                    sourceComponent: buttonIconComponent
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Loader {
                    sourceComponent: buttonTextComponent
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

            }

        }

        Component {
            id: textOnlyComponent

            Loader {
                sourceComponent: buttonTextComponent
                anchors.centerIn: parent
                anchors.fill: parent
            }

        }

        Component {
            id: iconOnlyComponent

            Loader {
                sourceComponent: buttonIconComponent
                anchors.centerIn: parent
                anchors.fill: parent
            }

        }

    }

    background: Rectangle {
        id: backgroundRect

        color: palette.button
        opacity: root.enabled ? 1 : 0.3
        visible: false
        radius: root.backgroundRadius
    }

}
