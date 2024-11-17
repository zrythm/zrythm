// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ToolBar {
    id: root

    required property var tracklist

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
                onTriggered: ActionController.createEmptyTrack(9)
            }

            MenuItem {
                text: qsTr("Add Audio Track")
                onTriggered: ActionController.createEmptyTrack(1)
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
                onTriggered: ActionController.createEmptyTrack(7)
            }

            MenuItem {
                text: qsTr("Add MIDI FX Track")
                onTriggered: ActionController.createEmptyTrack(10)
            }

            MenuSeparator {
            }

            MenuItem {
                text: qsTr("Add Audio Group Track")
                onTriggered: ActionController.createEmptyTrack(8)
            }

            MenuItem {
                text: qsTr("Add MIDI Group Track")
                onTriggered: ActionController.createEmptyTrack(11)
            }

            MenuSeparator {
            }

            MenuItem {
                text: qsTr("Add Folder Track")
                onTriggered: ActionController.createEmptyTrack(12)
            }

        }

    }

}
