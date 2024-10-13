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
        spacing: 0

        Rectangle {
            id: centerBox

            Layout.fillWidth: true
            Layout.fillHeight: true

            SplitView {
                id: mainSplitView

                anchors.fill: parent
                orientation: Qt.Horizontal

                ZrythmResizablePanel {
                    SplitView.fillHeight: true
                    SplitView.preferredWidth: 200
                    SplitView.minimumWidth: 30
                    title: "Browser"
                    vertical: false

                    content: Rectangle {
                        color: "#2C2C2C"

                        Label {
                            anchors.centerIn: parent
                            text: "Browser content"
                            color: "white"
                        }

                    }

                }

                SplitView {
                    id: centerSplitView

                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                    orientation: Qt.Vertical

                    Pane {
                        SplitView.fillWidth: true
                        SplitView.preferredHeight: 200
                        SplitView.minimumHeight: 30

                        Rectangle {
                            color: "#2C2C2C"

                            Label {
                                anchors.fill: parent
                                anchors.centerIn: parent
                                text: "Tracks content"
                                color: "white"
                            }

                        }

                    }

                    ZrythmResizablePanel {
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        SplitView.minimumHeight: 30
                        title: "Piano Roll"
                        vertical: true

                        content: Rectangle {
                            color: "#1C1C1C"

                            Label {
                                anchors.centerIn: parent
                                text: "Piano Roll content"
                                color: "white"
                            }

                        }

                    }

                }

                ZrythmResizablePanel {
                    SplitView.fillHeight: true
                    SplitView.preferredWidth: 200
                    SplitView.minimumWidth: 30
                    title: "Other"
                    vertical: false

                    content: Rectangle {
                        color: "#2C2C2C"

                        Label {
                            anchors.centerIn: parent
                            text: "Browser content"
                            color: "white"
                        }

                    }

                }

            }

        }

        Rectangle {
            id: botBar

            implicitHeight: 24
            color: "#2C2C2C"
            Layout.fillWidth: true

            Text {
                anchors.right: parent.right
                text: "Status Bar"
            }

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

                ToolSeparator {
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

                ToolSeparator {
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

                ToolSeparator {
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
