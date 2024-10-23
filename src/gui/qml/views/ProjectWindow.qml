// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import Qt.labs.platform as Labs
import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ApplicationWindow {
    id: root

    required property var project

    function closeAndDestroy() {
        console.log("Closing and destroying project window");
        close();
        destroy();
    }

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
                    visible: GlobalState.settingsManager.leftPanelVisible
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
                        project: root.project
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        SplitView.preferredHeight: 200
                        SplitView.minimumHeight: implicitHeight
                        Layout.verticalStretchFactor: 2
                    }

                    BottomDock {
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        SplitView.minimumHeight: 30
                        visible: GlobalState.settingsManager.bottomPanelVisible
                        Layout.verticalStretchFactor: 1
                    }

                }

                RightDock {
                    id: rightDock

                    SplitView.fillHeight: true
                    SplitView.preferredWidth: 200
                    SplitView.minimumWidth: 30
                    visible: GlobalState.settingsManager.rightPanelVisible
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

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")

            Action {
                text: qsTr("New Long Long Long Long Long Long Long Long Long Name")
            }

            Action {
                text: qsTr("Open")
            }

        }

        Menu {
            title: qsTr("&Edit")

            Action {
                text: qsTr("Undo")
            }

        }

        Menu {
            title: qsTr("&View")

            Menu {
                title: qsTr("Appearance")

                Action {
                    text: qsTr("Switch Light/Dark Theme")
                    onTriggered: {
                        Style.darkMode = !Style.darkMode;
                    }
                }

                Menu {
                    title: qsTr("Theme Color")

                    Action {
                        text: qsTr("Zrythm Orange")
                        onTriggered: {
                            Style.primaryColor = Style.zrythmColor;
                        }
                    }

                    Action {
                        text: qsTr("Celestial Blue")
                        onTriggered: {
                            Style.primaryColor = Style.celestialBlueColor;
                        }
                    }

                    Action {
                        text: qsTr("Jonquil Yellow")
                        onTriggered: {
                            Style.primaryColor = Style.jonquilYellowColor;
                        }
                    }

                }

            }

        }

        Menu {
            title: qsTr("&Help")

            Action {
                text: qsTr("About Zrythm")
                onTriggered: {
                }
            }

        }

    }

    header: MainToolbar {
        id: headerBar
    }

}
