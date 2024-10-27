// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Control {
    id: control

    required property var track
    readonly property real buttonHeight: 18
    readonly property real buttonPadding: 1

    hoverEnabled: true
    implicitWidth: 200
    implicitHeight: track.fullVisibleHeight
    opacity: Style.getOpacity(track.enabled, control.Window.active)

    background: Rectangle {
        color: {
            let c = control.palette.window;
            if (control.hovered)
                c = Style.getColorBlendedTowardsContrast(c);

            if (track.selected || control.down)
                c = Style.getColorBlendedTowardsContrast(c);

            return c;
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onTapped: function(point) {
                track.setExclusivelySelected(true);
            }
        }

    }

    contentItem: GridLayout {
        id: grid

        anchors.fill: parent
        rows: 2
        columns: 4

        Rectangle {
            id: trackColor

            width: 6
            Layout.fillHeight: true
            color: track.color
            Layout.row: 0
            Layout.column: 0
            Layout.rowSpan: 2
        }

        Label {
            id: trackNameLabel

            text: track.name
            Layout.row: 0
            Layout.column: 1
            Layout.columnSpan: 2
            Layout.topMargin: 2
            font: track.selected ? Style.buttonTextFont : Style.normalTextFont
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        }

        LinkedButtons {
            id: linkedButtons

            Layout.row: 1
            Layout.column: 1
            layer.enabled: true

            Button {
                text: "M"
                checkable: true
                styleHeight: control.buttonHeight
                padding: control.buttonPadding
            }

            Button {
                id: soloButton

                text: "S"
                checkable: true
                styleHeight: control.buttonHeight
                padding: control.buttonPadding
            }

            RecordButton {
                styleHeight: control.buttonHeight
                padding: control.buttonPadding
                height: soloButton.height
            }

            layer.effect: DropShadowEffect {
            }

        }

        LinkedButtons {
            id: bottomRightButtons

            layer.enabled: true

            Button {
                styleHeight: control.buttonHeight
                padding: control.buttonPadding
                checkable: true
                icon.source: Style.getIcon("zrythm-dark", "automation-4p.svg")
            }

            layer.effect: DropShadowEffect {
            }

        }

        RowLayout {
            id: meters

            Layout.row: 0
            Layout.column: 3
            Layout.rowSpan: 2
            spacing: 0

            Rectangle {
                width: 4
                color: "red"
                Layout.fillHeight: true
            }

            Rectangle {
                width: 4
                color: "green"
                Layout.fillHeight: true
            }

        }

    }

}
