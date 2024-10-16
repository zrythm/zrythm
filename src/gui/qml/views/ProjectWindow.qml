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

                LeftDock {
                    SplitView.fillHeight: true
                    SplitView.preferredWidth: 200
                    SplitView.minimumWidth: 30
                }

                SplitView {
                    // Pane {
                    //     SplitView.fillWidth: true
                    //     SplitView.preferredHeight: 200
                    //     SplitView.minimumHeight: 30
                    // }

                    id: centerSplitView

                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                    orientation: Qt.Vertical

                    CenterDock {
                    }

                    BottomDock {
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        SplitView.minimumHeight: 30
                    }

                }

                RightDock {
                    SplitView.fillHeight: true
                    SplitView.preferredWidth: 200
                    SplitView.minimumWidth: 30
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
                font: root.font
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
                text: qsTr("Undo")
                onTriggered: {
                }
            }

        }

    }

    header: MainToolbar {
        id: headerBar
    }

}
