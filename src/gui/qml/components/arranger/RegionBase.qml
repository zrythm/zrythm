// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ArrangerObjectBase {
    id: root

    implicitWidth: 10
    implicitHeight: 10
    Component.onCompleted: {
        console.log(arrangerObject.name);
    }

    Rectangle {
        id: backgroundRect

        color: {
            let c = arrangerObject.hasColor ? arrangerObject.color : track.color;
            return Style.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.visualFocus, root.down);
        }
        anchors.fill: parent
        radius: Style.toolButtonRadius
    }

    Rectangle {
        id: nameRect

        color: root.palette.button
        width: Math.min(textMetrics.width + 2 * Style.buttonPadding, root.width - Style.toolButtonRadius)
        height: Math.min(textMetrics.height + 2 * Style.buttonPadding, root.height - Style.toolButtonRadius)
        bottomRightRadius: Style.toolButtonRadius
        topLeftRadius: Style.toolButtonRadius

        anchors {
            left: parent.left
            top: parent.top
        }

        Text {
            id: nameText

            text: arrangerObject.name
            color: root.palette.buttonText
            font: root.font
            anchors.fill: parent
            padding: Style.buttonPadding
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        TextMetrics {
            id: textMetrics

            text: nameText.text
            font: nameText.font
        }

    }

    // Left resize handle
    Rectangle {
        id: leftHandle

        width: 5
        z: 10
        height: parent.height
        color: Style.backgroundAppendColor
        visible: root.hovered || leftDrag.drag.active
        topLeftRadius: Style.toolButtonRadius
        bottomLeftRadius: Style.toolButtonRadius

        MouseArea {
            id: leftDrag

            property real startX

            acceptedButtons: Qt.LeftButton
            anchors.fill: parent
            anchors.margins: -5
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            onPressed: (mouse) => {
                startX = root.x;
            }
            onPositionChanged: (mouse) => {
                if (drag.active) {
                    let ticksDiff = (root.x - startX) / ruler.pxPerTick;
                    arrangerObject.position.ticks += ticksDiff;
                    if (arrangerObject.hasLength) {
                        arrangerObject.clipStartTicks += ticksDiff;
                        arrangerObject.endPosition.ticks -= ticksDiff;
                    }
                    startX = root.x;
                    parent.x = 0;
                }
            }
        }

    }

    // Right resize handle
    Rectangle {
        id: rightHandle

        width: 5
        height: parent.height
        anchors.right: parent.right
        color: Style.backgroundAppendColor
        visible: root.hovered || rightDrag.drag.active
        z: 10
        topRightRadius: Style.toolButtonRadius
        bottomRightRadius: Style.toolButtonRadius

        MouseArea {
            id: rightDrag

            property real startX

            acceptedButtons: Qt.LeftButton
            anchors.fill: parent
            anchors.margins: -5
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            onPressed: (mouse) => {
                startX = mouse.x;
            }
            onPositionChanged: (mouse) => {
                if (drag.active) {
                    let ticksDiff = (mouse.x - startX) / ruler.pxPerTick;
                    if (arrangerObject.hasLength)
                        arrangerObject.endPosition.ticks += ticksDiff;

                }
            }
        }

    }

}
