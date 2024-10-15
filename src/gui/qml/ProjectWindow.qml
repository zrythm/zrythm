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

    palette {
        accent: themeManager().accent
        base: themeManager().base
        brightText: themeManager().base
        button: themeManager().base.lighter().lighter(1.2)
        buttonText: "white"
        dark: "white" // used by paginator
        light: "#999999" // combo box hover background
        highlight: themeManager().accent
        link: themeManager().accent
        placeholderText: themeManager().base.lighter().lighter(1.2)
        text: "white"
        window: themeManager().base
        windowText: "white"
    }

    Loader {
        id: menuLoader

        sourceComponent: useLabsMenuBar ? null : regularMenuBar
    }

    Component {
        id: regularMenuBar

        ZrythmMenuBar {
            ZrythmMenu {
                title: qsTr("&File")

                Action {
                    text: qsTr("New Long Long Long Long Long Long Long Long Long Name")
                }

                Action {
                    text: qsTr("Open")
                }

            }

            ZrythmMenu {
                title: qsTr("&Edit")

                Action {
                    text: qsTr("Undo")
                }

            }

            ZrythmMenu {
                title: qsTr("&Test")

                ZrythmMenuItem {
                    text: qsTr("Something")
                    onTriggered: {
                    }
                }

                Action {
                    text: "Copy"
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

                        ZrythmArranger {
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

    header: ZrythmToolBar {
        id: headerBar

        leftItems: [
            ZrythmToolButton {
                // TODO tooltip
                // text: qsTr("Toggle Left Panel")

                id: toggleLeftDock

                checkable: true
                iconSource: Qt.resolvedUrl("icons/gnome-icon-library/dock-left-symbolic.svg")
            },
            ZrythmToolSeparator {
            },
            ZrythmSplitButton {
                id: undoBtn

                // text: qsTr("Undo")
                iconSource: Qt.resolvedUrl("icons/zrythm-dark/edit-undo.svg")

                menuItems: ZrythmMenu {
                    Action {
                        text: qsTr("Undo Move")
                    }

                }

            },
            ZrythmSplitButton {
                id: redoBtn

                // text: qsTr("Redo")
                iconSource: Qt.resolvedUrl("icons/zrythm-dark/edit-redo.svg")
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
                // TODO tooltip
                // text: qsTr("Toggle Left Panel")

                id: toggleRightDock

                checkable: true
                iconSource: Qt.resolvedUrl("icons/gnome-icon-library/dock-right-symbolic.svg")
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

}
