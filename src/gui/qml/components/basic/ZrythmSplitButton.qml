// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import ZrythmStyle 1.0

Item {
    id: root

    property alias text: mainButton.text
    property alias iconSource: mainButton.icon.source
    property alias menuItems: menuLoader.sourceComponent
    property color separatorColor: Qt.lighter(palette.button, 1.3)
    property bool directionUpward: false
    property alias tooltipText: mainButtonTooltip.text
    property alias menuTooltipText: arrowButtonTooltip.text

    signal clicked()

    implicitWidth: rowLayout.implicitWidth
    implicitHeight: 24

    RowLayout {
        id: rowLayout

        anchors.fill: parent
        spacing: 0

        ToolButton {
            // for testing
            // iconSource: Qt.resolvedUrl("../icons/zrythm-dark/edit-undo.svg")

            id: mainButton

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            onClicked: root.clicked()
            clip: true
            anchors.rightMargin: -width

            ToolTip {
                id: mainButtonTooltip
            }

            Component.onCompleted: {
                background.anchors.fill = mainButton
                background.anchors.rightMargin = -mainButton.width
            }

        }

        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: root.separatorColor
        }

        ToolButton {
            id: arrowButton

            Layout.fillHeight: true
            // Layout.preferredWidth: root.height
            text: root.directionUpward ? "▲" : "▼"
            font.pointSize: 6
            onClicked: menuLoader.item.open()
            clip: true
            anchors.leftMargin: -width
            Layout.alignment: Qt.AlignHCenter
            width: 12

            ToolTip {
                id: arrowButtonTooltip
                text: qsTr("More Options...")
            }

              Component.onCompleted: {
                background.anchors.fill = arrowButton
                background.anchors.leftMargin = -arrowButton.width
            }

        }

    }

    Loader {
        id: menuLoader

        y: root.directionUpward ? -menuLoader.item.height : root.height

        sourceComponent: Menu {
            Action {
                text: "Fill Me Fill Me Fill Me Fill Me Fill Me Fill Me Fill Me Fill Me Fill Me"
            }

            Action {
                text: "Fill Me2"
            }

            Action {
                text: "Fill Me3"
            }

        }

    }

}
