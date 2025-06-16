// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ToolBar {
    id: root

    required property var tracklist
    required property var trackFactory

    ToolButton {
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "add.svg")

        ToolTip {
            text: qsTr("Add track")
        }

        onClicked: {
            addMenu.popup();
        }

        Menu {
            id: addMenu

            MenuItem {
                text: qsTr("Add _MIDI Track")
                onTriggered: trackFactory.addEmptyTrackFromType(8)
            }

            MenuItem {
                text: qsTr("Add Audio Track")
                onTriggered: trackFactory.addEmptyTrackFromType(1)
            }

            MenuSeparator {
            }

            MenuItem {
                text: qsTr("Import File...")
            }

            MenuSeparator {
            }

            MenuItem {
                text: qsTr("Add Audio FX Track")
                onTriggered: trackFactory.addEmptyTrackFromType(6)
            }

            MenuItem {
                text: qsTr("Add MIDI FX Track")
                onTriggered: trackFactory.addEmptyTrackFromType(9)
            }

            MenuSeparator {
            }

            MenuItem {
                text: qsTr("Add Audio Group Track")
                onTriggered: trackFactory.addEmptyTrackFromType(7)
            }

            MenuItem {
                text: qsTr("Add MIDI Group Track")
                onTriggered: trackFactory.addEmptyTrackFromType(10)
            }

            MenuSeparator {
            }

            MenuItem {
                text: qsTr("Add Folder Track")
                onTriggered: trackFactory.addEmptyTrackFromType(11)
            }

        }

    }

}
