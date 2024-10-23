// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Rectangle {
    id: root

    required property var track

    implicitWidth: 200
    implicitHeight: 60

    GridLayout {
        id: grid

        anchors.fill: parent
        rows: 2
        columns: 3

        Rectangle {
            id: trackColorAndIcon

            width: 32
            height: 60
            color: track.color
            Layout.row: 0
            Layout.column: 0
            Layout.rowSpan: 2
        }

        Label {
            text: track.name
            Layout.row: 0
            Layout.column: 1
        }

    }

    Component.onCompleted: {
        console.log("TrackView:", track, typeof(track));
    }

}
