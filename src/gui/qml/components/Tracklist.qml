// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Item {
    id: root

    required property var tracklist
    required property bool pinned
    readonly property real contentHeight: listView.contentHeight + (pinned ? 0 : dropSpace.height)

    ListView {
        id: listView

        implicitWidth: 200
        implicitHeight: 200

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: root.pinned ? parent.bottom : dropSpace.top
        }

        model: TrackFilterProxyModel {
            sourceModel: tracklist
            Component.onCompleted: {
                addVisibilityFilter(true);
                addPinnedFilter(root.pinned);
            }
        }

        delegate: TrackView {
            width: ListView.view.width
        }

    }

    Item {
        id: dropSpace

        visible: !root.pinned
        height: 40

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: contextMenu.popup()
        }

        Menu {
            id: contextMenu

            MenuItem {
                text: qsTr("Test")
                onTriggered: console.log("Clicked")
            }

        }

    }

}
