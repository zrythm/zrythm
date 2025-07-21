// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle

Control {
    id: root

    required property var arrangerObject
    required property var track
    required property real pxPerTick
    readonly property alias down: dragArea.pressed
    readonly property color objectColor: {
        let c = arrangerObject.hasColor ? arrangerObject.color : track.color;
        if (arrangerObject.selected)
            c = Style.getColorBlendedTowardsContrastByFactor(c, 1.1);

        return Style.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.visualFocus, root.down);
    }
    signal arrangerObjectClicked

    font: arrangerObject.selected ? Style.arrangerObjectBoldTextFont : Style.arrangerObjectTextFont
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    x: arrangerObject.position.ticks * pxPerTick
    width: 100
    height: track.height

    ContextMenu.menu: Menu {
        MenuItem {
            text: qsTr("Test")
            onTriggered: console.log("Clicked")
        }
    }

    MouseArea {
        id: dragArea

        property real startX

        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        drag.target: parent
        drag.axis: Drag.XAxis
        drag.threshold: 0
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        onPressed: mouse => {
            startX = root.x;
            root.forceActiveFocus();
            root.arrangerObjectClicked();
        }
        onPositionChanged: mouse => {
            if (drag.active) {
                if (mouse.button == Qt.LeftButton) {
                    let newTicks = root.x / root.pxPerTick;
                    let arrangerObject = root.arrangerObject;
                    let ticksDiff = newTicks - arrangerObject.position.ticks;
                    arrangerObject.position.ticks = newTicks;
                    if (arrangerObject.hasLength)
                        arrangerObject.endPosition.ticks += ticksDiff;
                }
            }
        }
        onReleased: {}
    }
}
