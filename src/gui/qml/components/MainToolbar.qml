// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Zrythm 1.0

ZrythmToolBar {
    id: headerBar

    leftItems: [
        ZrythmToolButton {
            id: toggleLeftDock

            tooltipText: qsTr("Toggle Left Panel")
            checkable: true
            iconSource: Qt.resolvedUrl("../icons/gnome-icon-library/dock-left-symbolic.svg")
        },
        ZrythmToolSeparator {
        },
        ZrythmSplitButton {
            id: undoBtn

            tooltipText: qsTr("Undo")
            menuTooltipText: qsTr("Undo Multiple")
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/edit-undo.svg")

            menuItems: ZrythmMenu {
                Action {
                    text: qsTr("Undo Move")
                }

            }

        },
        ZrythmSplitButton {
            id: redoBtn

            tooltipText: qsTr("Redo")
            menuTooltipText: qsTr("Redo Multiple")
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/edit-redo.svg")
            enabled: false
        },
        ZrythmToolSeparator {
        },
        // Implement ToolboxWidget
        Rectangle {
            // Add ToolboxWidget properties and functionality

            id: toolbox
        }
    ]
    rightItems: [
        ZrythmToolButton {
            id: toggleRightDock

            tooltipText: qsTr("Toggle Right Panel")
            checkable: true
            iconSource: Qt.resolvedUrl("../icons/gnome-icon-library/dock-right-symbolic.svg")
        },
        ZrythmToolSeparator {
        },
        ZrythmToolButton {
            id: menuButton

            text: qsTr("Menu")
            icon.name: "open-menu-symbolic"
            onClicked: primaryMenu.open()

            ZrythmMenu {
                id: primaryMenu

                Action {
                    text: qsTr("Open a Project…")
                }

                ZrythmMenuItem {
                    // Implement new project action

                    text: qsTr("Create New Project…")
                    onTriggered: {
                    }
                }

                ZrythmMenuSeparator {
                }

                ZrythmMenuItem {
                    // Implement save action

                    text: qsTr("Save")
                    onTriggered: {
                    }
                }

                ZrythmMenuItem {
                    // Implement save as action

                    text: qsTr("Save As…")
                    onTriggered: {
                    }
                }

                ZrythmMenuSeparator {
                }

                ZrythmMenuItem {
                    // Implement export as action

                    text: qsTr("Export As…")
                    onTriggered: {
                    }
                }

                ZrythmMenuItem {
                    // Implement export graph action

                    text: qsTr("Export Graph…")
                    onTriggered: {
                    }
                }

                ZrythmMenuSeparator {
                }

                ZrythmMenuItem {
                    text: qsTr("Fullscreen")
                    onTriggered: {
                        root.visibility = root.visibility === Window.FullScreen ? Window.AutomaticVisibility : Window.FullScreen;
                    }
                }

                ZrythmMenuSeparator {
                }

                ZrythmMenuItem {
                    // Open preferences dialog

                    text: qsTr("Preferences")
                    onTriggered: {
                    }
                }

                ZrythmMenuItem {
                    // Show keyboard shortcuts

                    text: qsTr("Keyboard Shortcuts")
                    onTriggered: {
                    }
                }

                ZrythmMenuItem {
                    // Show about dialog

                    text: qsTr("About Zrythm")
                    onTriggered: {
                    }
                }

            }

        }
    ]
}
