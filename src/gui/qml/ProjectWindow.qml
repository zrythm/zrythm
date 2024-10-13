// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Zrythm 1.0

ApplicationWindow {
    id: root

    required property var project

    function closeAndDestroy() {
        console.log("Closing and destroying project window");
        close();
        destroy();
    }

    title: qsTr("Zrythm")
    visible: true
    onClosing: {
    }
    Component.onCompleted: {
        console.log("ApplicationWindow created");
        project.aboutToBeDeleted.connect(closeAndDestroy);
    }

    ToolBar {
        id: headerBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        RowLayout {
            anchors.fill: parent

            RowLayout {
                id: headerStartBox

                ToolButton {
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

                Button {
                    // Perform undo action

                    id: undoBtn

                    text: qsTr("Undo")
                    icon.name: "edit-undo"
                    enabled: false
                    onClicked: {
                    }
                }

                Button {
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

            Label {
                id: windowTitle

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: root.title
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

                ToolButton {
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

}
