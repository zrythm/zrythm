// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import Qt.labs.platform as Labs
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Zrythm 1.0

ApplicationWindow {
    id: root

    required property var project
    readonly property bool useLabsMenuBar: Qt.platform.os === "osx"

    function closeAndDestroy() {
        console.log("Closing and destroying project window");
        close();
        destroy();
    }

    menuBar: useLabsMenuBar ? null : menuLoader.item
    title: project.title
    width: 1280
    height: 720
    visible: true
    onClosing: {
    }
    Component.onCompleted: {
        console.log("ApplicationWindow created on platform", Qt.platform.os);
        project.aboutToBeDeleted.connect(closeAndDestroy);
    }

    Loader {
        id: menuLoader

        sourceComponent: useLabsMenuBar ? null : regularMenuBar
    }

    Component {
        id: regularMenuBar

        ZrythmMenuBar {
            Menu {
                title: qsTr("&File")

                MenuItem {
                    // Implement new project action

                    text: qsTr("New")
                    onTriggered: {
                    }
                }

                MenuItem {
                    // Implement open action

                    text: qsTr("Open")
                    onTriggered: {
                    }
                }

            }

            Menu {
                title: qsTr("&Edit")

                MenuItem {
                    // Implement undo action

                    text: qsTr("Undo")
                    onTriggered: {
                    }
                }

            }

        }

    }

    ColumnLayout {
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Rectangle {
            id: centerBox

            Layout.fillWidth: true
            Layout.fillHeight: true

            // Implement CenterDockWidget
            Rectangle {
                // Add CenterDockWidget properties and functionality

                id: centerDock

                anchors.fill: parent
            }

            // Implement ToastOverlay
            Rectangle {
                // Add ToastOverlay properties and functionality

                id: toastOverlay

                anchors.fill: parent
            }

        }

        // Implement BotBarWidget
        Rectangle {
            // Add BotBarWidget properties and functionality

            id: botBar

            Layout.fillWidth: true
        }

    }

    Labs.MenuBar {
        Labs.Menu {
            title: qsTr("&File")

            Labs.MenuItem {
                // Implement new project action

                text: qsTr("New")
                onTriggered: {
                }
            }

            Labs.MenuItem {
                // Implement open action

                text: qsTr("Open")
                onTriggered: {
                }
            }

        }

        Labs.Menu {
            title: qsTr("&Edit")

            Labs.MenuItem {
                // Implement undo action

                // Implement undo action
                text: qsTr("Undo")
                onTriggered: {
                }
            }

        }

    }

    header: ToolBar {
        id: headerBar

        RowLayout {
            anchors.fill: parent

            RowLayout {
                id: headerStartBox

                ZrythmToolButton {
                    // Toggle left panel

                    id: startDockSwitcher

                    // Implement PanelToggleButton functionality
                    text: qsTr("Toggle Left Panel")
                    onClicked: {
                    }
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: "gray"
                }

                ZrythmToolButton {
                    // Perform undo action

                    id: undoBtn

                    text: qsTr("Undo")
                    icon.name: "edit-undo"
                    enabled: false
                    onClicked: {
                    }
                }

                ZrythmToolButton {
                    // Perform redo action

                    id: redoBtn

                    text: qsTr("Redo")
                    icon.name: "edit-redo"
                    enabled: false
                    onClicked: {
                    }
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: "gray"
                }

                // Implement ToolboxWidget
                Rectangle {
                    // Add ToolboxWidget properties and functionality

                    id: toolbox
                }

            }

            RowLayout {
                id: headerEndBox

                ToolButton {
                    // Toggle right panel

                    id: endDockSwitcher

                    // Implement PanelToggleButton functionality
                    text: qsTr("Toggle Right Panel")
                    onClicked: {
                    }
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: "gray"
                }

                ZrythmToolButton {
                    id: menuButton

                    text: qsTr("Menu")
                    icon.name: "open-menu-symbolic"
                    onClicked: primaryMenu.open()

                    Menu {
                        id: primaryMenu

                        MenuItem {
                            // Implement open action

                            text: qsTr("Open a Project…")
                            onTriggered: {
                            }
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

                            text: qsTr("About Zrythm")
                            onTriggered: {
                            }
                        }

                    }

                }

            }

        }

    }

}
