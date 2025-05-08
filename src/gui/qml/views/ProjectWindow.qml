// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import "../config.js" as Config
import QtQuick
import QtQuick.Controls
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
        project.activate();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        SplitView {
            id: mainSplitView

            Layout.fillWidth: true
            Layout.fillHeight: true
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
                    project: root.project
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

        Item {
            id: botBar

            implicitHeight: 24
            Layout.fillWidth: true

            Text {
                anchors.right: parent.right
                text: "Status Bar"
                font: root.font
            }

        }

    }

    menuBar: MainMenuBar {
        id: mainMenuBar
    }

    header: MainToolbar {
        id: headerBar

        project: root.project
    }

}
