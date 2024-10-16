// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property alias text: mainButton.text
    property alias iconSource: mainButton.iconSource
    property alias menuItems: menuLoader.sourceComponent
    property color separatorColor: Qt.lighter(palette.button, 1.3)
    property bool directionUpward: false
    property alias tooltipText: mainButton.tooltipText
    property alias menuTooltipText: arrowButton.tooltipText

    signal clicked()

    implicitWidth: rowLayout.implicitWidth
    implicitHeight: 24

    RowLayout {
        id: rowLayout

        anchors.fill: parent
        spacing: 0

        ZrythmToolButton {
            // for testing
            // iconSource: Qt.resolvedUrl("../icons/zrythm-dark/edit-undo.svg")

            id: mainButton

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            onClicked: root.clicked()
            clip: true
            anchors.rightMargin: -width

            background: Rectangle {
                anchors.fill: mainButton
                anchors.rightMargin: -mainButton.width
                radius: mainButton.backgroundRadius
                visible: false
            }

        }

        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: root.separatorColor
        }

        ZrythmToolButton {
            id: arrowButton

            Layout.fillHeight: true
            // Layout.preferredWidth: root.height
            text: root.directionUpward ? "▲" : "▼"
            font.pointSize: 6
            onClicked: menuLoader.item.open()
            clip: true
            anchors.leftMargin: -width
            Layout.alignment: Qt.AlignHCenter
            tooltipText: qsTr("More Options...")

            background: Rectangle {
                anchors.fill: arrowButton
                anchors.leftMargin: -arrowButton.width
                radius: arrowButton.backgroundRadius
                visible: false
            }

        }

    }

    Loader {
        id: menuLoader

        y: root.directionUpward ? -menuLoader.item.height : root.height

        sourceComponent: ZrythmMenu {
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
