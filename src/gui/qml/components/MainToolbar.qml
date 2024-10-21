// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import ZrythmStyle 1.0
import Zrythm 1.0

ZrythmToolBar {
    id: headerBar

    property alias leftDockVisible: toggleLeftDock.checked
    property alias rightDockVisible: toggleRightDock.checked

    leftItems: [
        ToolButton {
            id: toggleLeftDock

            ToolTip {
                text: qsTr("Toggle Left Panel")
            }
            checkable: true
            icon.source: Qt.resolvedUrl("../icons/gnome-icon-library/dock-left-symbolic.svg")
        },
        ToolSeparator {
        },
        ZrythmSplitButton {
            id: undoBtn

            tooltipText: qsTr("Undo")
            menuTooltipText: qsTr("Undo Multiple")
            iconSource: Style.getIcon("zrythm-dark", "edit-undo.svg")

            menuItems: Menu {
                Action {
                    text: qsTr("Undo Move")
                }

            }

        },
        ZrythmSplitButton {
            id: redoBtn

            tooltipText: qsTr("Redo")
            menuTooltipText: qsTr("Redo Multiple")
            iconSource: Style.getIcon("zrythm-dark", "edit-redo.svg")
            enabled: false
        },
        ToolSeparator {
        },
        // Implement ToolboxWidget
        Rectangle {
            // Add ToolboxWidget properties and functionality

            id: toolbox
        }
    ]
    rightItems: [
        ToolButton {
            id: toggleRightDock

            ToolTip {
                text: qsTr("Toggle Right Panel")
            }

            checkable: true
            icon.source: Qt.resolvedUrl("../icons/gnome-icon-library/dock-right-symbolic.svg")
        },
        ToolSeparator {
        },
        ToolButton {
            id: menuButton

            text: qsTr("Menu")
            icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/gnome-icon-library/open-menu-symbolic.svg")
            onClicked: primaryMenu.open()

            Menu {
                id: primaryMenu

                Action {
                    text: qsTr("Open a Project…")
                }

                MenuItem {
                    // Implement new project action

                    text: qsTr("Create New Project…")
                    onTriggered: {
                    }
                }

                MenuSeparator {
                }

                MenuItem {
                    // Implement save action

                    text: qsTr("Save")
                    onTriggered: {
                    }
                }

                MenuItem {
                    // Implement save as action

                    text: qsTr("Save As…")
                    onTriggered: {
                    }
                }

                MenuSeparator {
                }

                MenuItem {
                    // Implement export as action

                    text: qsTr("Export As…")
                    onTriggered: {
                    }
                }

                MenuItem {
                    // Implement export graph action

                    text: qsTr("Export Graph…")
                    onTriggered: {
                    }
                }

                MenuSeparator {
                }

                MenuItem {
                    text: qsTr("Fullscreen")
                    onTriggered: {
                        root.visibility = root.visibility === Window.FullScreen ? Window.AutomaticVisibility : Window.FullScreen;
                    }
                }

                MenuSeparator {
                }

                MenuItem {
                    // Open preferences dialog

                    text: qsTr("Preferences")
                    onTriggered: {
                    }
                }

                MenuItem {
                    // Show keyboard shortcuts

                    text: qsTr("Keyboard Shortcuts")
                    onTriggered: {
                    }
                }

                MenuItem {
                    // Show about dialog

                    text: qsTr("About Zrythm Long Long Long Long Long Long Long")
                    onTriggered: {
                    }
                }

            }

        }
    ]
}
