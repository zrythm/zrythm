// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Control {
    id: root

    required property var arrangerObject
    required property var track
    required property var ruler
    property var contextMenu
    readonly property alias down: dragArea.pressed
    readonly property color objectColor: {
        let c = arrangerObject.hasColor ? arrangerObject.color : track.color;
        if (arrangerObject.selected)
            c = Style.getColorBlendedTowardsContrastByFactor(c, 1.1);

        return Style.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.visualFocus, root.down);
    }

    font: arrangerObject.selected ? Style.arrangerObjectBoldTextFont : Style.arrangerObjectTextFont
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    x: arrangerObject.position.ticks * ruler.pxPerTick
    width: 100
    height: track.height

    MouseArea {
        id: dragArea

        property real startX

        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        drag.target: parent
        drag.axis: Drag.XAxis
        drag.threshold: 0
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        onPressed: (mouse) => {
            startX = root.x;
            root.forceActiveFocus();
            if (mouse.button === Qt.RightButton)
                contextMenu.popup();

        }
        onPositionChanged: (mouse) => {
            if (drag.active) {
                if (mouse.button == Qt.LeftButton) {
                    let newTicks = root.x / ruler.pxPerTick;
                    let ticksDiff = newTicks - arrangerObject.position.ticks;
                    arrangerObject.position.ticks = newTicks;
                    if (arrangerObject.hasLength)
                        arrangerObject.endPosition.ticks += ticksDiff;

                }
            }
        }
        onReleased: {
            // let finalTicks = root.x / ruler.pxPerTick;
            // arrangerObject.position.ticks = finalTicks;
        }
    }

}
