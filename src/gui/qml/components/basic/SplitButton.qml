// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import ZrythmStyle 1.0

// upward popup TODO
ColumnLayout {
    id: root

    property alias text: mainButton.text
    property alias iconSource: mainButton.icon.source
    property alias menuItems: menuLoader.sourceComponent
    property bool directionUpward: false
    property alias tooltipText: mainButtonTooltip.text
    property alias menuTooltipText: arrowButtonTooltip.text

    signal clicked()

    spacing: 0

    LinkedButtons {
        id: rowLayout

        Layout.fillWidth: true
        Layout.fillHeight: true

        Button {
            id: mainButton

            onClicked: root.clicked()

            ToolTip {
                id: mainButtonTooltip
            }

        }

        Button {
            id: arrowButton

            text: root.directionUpward ? "▲" : "▼"
            font.pointSize: 6
            onClicked: menuLoader.item.open()
            leftPadding: 0
            rightPadding: 0
            Layout.preferredWidth: 16

            ToolTip {
                id: arrowButtonTooltip

                text: qsTr("More Options...")
            }

        }

    }

    Loader {
        id: menuLoader

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
