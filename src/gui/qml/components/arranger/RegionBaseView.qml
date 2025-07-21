// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmStyle
import ZrythmGui

ArrangerObjectBaseView {
    id: root

    required property ClipEditor clipEditor
    property string regionName: arrangerObject.regionMixin.name.name

    onArrangerObjectClicked: {
        console.log("region clicked, setting clip editor region to ", regionName);
        clipEditor.setRegion(arrangerObject, root.track);
    }

    implicitWidth: 10
    implicitHeight: 10

    Rectangle {
        id: backgroundRect

        color: root.objectColor
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

            text: root.arrangerObject.regionMixin.name.name
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
            onPressed: mouse => {
                startX = root.x;
            }
            onPositionChanged: mouse => {
                if (drag.active) {
                    let ticksDiff = (root.x - startX) / root.pxPerTick;
                    root.arrangerObject.position.ticks += ticksDiff;
                    if (root.arrangerObject.hasLength) {
                        root.arrangerObject.clipStartTicks += ticksDiff;
                        root.arrangerObject.endPosition.ticks -= ticksDiff;
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
            onPressed: mouse => {
                startX = mouse.x;
            }
            onPositionChanged: mouse => {
                if (drag.active) {
                    let ticksDiff = (mouse.x - startX) / root.pxPerTick;
                    root.arrangerObject.regionMixin.bounds.length.ticks += ticksDiff;
                }
            }
        }
    }
}
