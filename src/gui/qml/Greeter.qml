// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0
import "config.js" as Config

ApplicationWindow {
    id: root

    function pluginManager() {
        return GlobalState.zrythm.pluginManager;
    }

    function pluginScanner() {
        return root.pluginManager().scanner;
    }

    function settingsManager() {
        return GlobalState.settingsManager;
    }

    function projectManager() {
        return GlobalState.projectManager;
    }

    function alertManager() {
        return GlobalState.alertManager;
    }

    function deviceManager() {
        return GlobalState.deviceManager;
    }

    function openProjectWindow(project) {
        let component = Qt.createComponent("views/ProjectWindow.qml");
        if (component.status === Component.Ready) {
            let newWindow = component.createObject(project, {
                "project": project,
                "deviceManager": deviceManager()
            });
            newWindow.show();
            root.close();
        } else {
            console.error("Error loading component:", component.errorString());
        }
    }

    title: "Zrythm"
    modality: Qt.ApplicationModal
    minimumWidth: 256
    width: 640
    height: 420
    visible: true
    font.family: Style.fontFamily
    font.pointSize: Style.fontPointSize

    Item {
        id: flatpakPage

        PlaceholderPage {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "flatpak-symbolic.svg")
            title: qsTr("About Flatpak")
            description: qsTr("Only audio plugins installed via Flatpak are supported.")
        }

    }

    Item {
        id: donationPage

        PlaceholderPage {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "credit-card-symbolic.svg")
            title: qsTr("Donate")
            description: qsTr("Zrythm relies on donations and purchases to sustain development. If you enjoy the software, please consider %1donating%2 or %3buying an installer%2.").arg("<a href=\"" + Config.DONATION_URL + "\">").arg("</a>").arg("<a href=\"" + Config.PURCHASE_URL + "\">")
        }

    }

    Item {
        id: proceedToConfigPage

        PlaceholderPage {
            title: qsTr("All Ready!")

            action: Action {
                text: qsTr("Proceed to Configuration")
                onTriggered: stack.push(configPage)
            }

        }

    }

    StackView {
        id: stack

        anchors.fill: parent
        initialItem: root.settingsManager().first_run ? firstRunPage : progressPage

        Component {
            id: firstRunPage

            Page {
                title: qsTr("Welcome")

                SwipeView {
                    id: welcomeCarousel

                    anchors.fill: parent
                    clip: true
                    Component.onCompleted: {
                        if (!Config.IS_INSTALLER_VER || Config.IS_TRIAL_VER)
                            addItem(donationPage);

                        if (Config.FLATPAK_BUILD)
                            addItem(flatpakPage);

                        addItem(proceedToConfigPage);
                    }

                    Item {
                        PlaceholderPage {
                            icon.source: ResourceManager.getIconUrl("zrythm-dark", "zrythm.svg")
                            title: qsTr("Welcome")
                            description: qsTr("Welcome to the Zrythm digital audio workstation. Move to the next page to get started.")
                        }

                    }

                    Item {
                        PlaceholderPage {
                            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "open-book-symbolic.svg")
                            title: qsTr("Read the Manual")
                            description: qsTr("If this is your first time using Zrythm, we suggest going through the 'Getting Started' section in the %1user manual%2.").arg("<a href=\"" + Config.USER_MANUAL_URL + "\">").arg("</a>")
                        }

                    }

                }

                // Add left navigation button
                RoundButton {
                    id: leftNavButton
                    readonly property real squareSize: 48
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "<"
                    onClicked: welcomeCarousel.decrementCurrentIndex()
                    visible: welcomeCarousel.currentIndex > 0
                    font.pixelSize: 18
                    font.bold: true
                    width: squareSize
                    height: squareSize
                }

                // Add right navigation button
                RoundButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: ">"
                    onClicked: welcomeCarousel.incrementCurrentIndex()
                    visible: welcomeCarousel.currentIndex < welcomeCarousel.count - 1
                    font.pixelSize: 18
                    font.bold: true
                    width: leftNavButton.squareSize
                    height: leftNavButton.squareSize
                }

                PageIndicator {
                    id: indicator

                    count: welcomeCarousel.count
                    currentIndex: welcomeCarousel.currentIndex
                    anchors.bottom: welcomeCarousel.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    interactive: true
                    onCurrentIndexChanged: welcomeCarousel.currentIndex = currentIndex
                }

            }

        }

        Component {
            id: configPage

            Page {
                // Add configuration content here

                title: qsTr("Configuration")

                ZrythmPreferencesPage {
                    title: qsTr("Initial Configuration")
                    anchors.fill: parent

                    ZrythmActionRow {
                        title: "Language"
                        subtitle: "Preferred language"

                        ComboBox {
                            model: ["English", "Spanish", "French"]
                        }

                    }

                    ZrythmActionRow {
                        title: "User Path"
                        subtitle: "Location to save user files"

                        ZrythmFilePicker {
                        }

                    }

                }

                header: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        ToolButton {
                            text: qsTr("‹")
                            onClicked: stack.pop()
                        }

                        Label {
                            text: qsTr("Configuration")
                            font.bold: true
                            elide: Text.ElideRight
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }

                    }

                }

                footer: DialogButtonBox {
                    standardButtons: DialogButtonBox.Reset
                    onReset: {
                        console.log("Resetting settings...");
                    }
                    horizontalPadding: 10

                    Button {
                        text: qsTr("Continue")
                        highlighted: true
                        onClicked: {
                            console.log("Proceeding to next page...");
                            root.settingsManager().first_run = false;
                            stack.push(progressPage);
                        }
                        DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    }

                }

            }

        }

        Component {
            id: progressPage

            Page {
                id: progressPagePage

                title: qsTr("Progress")
                StackView.onActivated: {
                    // start the scan
                    root.pluginManager().beginScan();
                }

                Connections {
                    function onScanFinished() {
                        stack.push(projectSelectorPage);
                    }

                    target: root.pluginManager()
                }

                PlaceholderPage {
                    icon.source: ResourceManager.getIconUrl("zrythm-dark", "zrythm-monochrome.svg")
                    title: qsTr("Scanning Plugins")
                }

                ColumnLayout {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 26
                    spacing: 12

                    Text {
                        id: scanProgressLabel

                        text: qsTr("Scanning:") + " " + root.pluginManager().currentlyScanningPlugin
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        Layout.fillWidth: true
                        color: palette.text
                        font.pointSize: 8
                        opacity: 0.6
                    }

                    ProgressBar {
                        id: scanProgressBar

                        indeterminate: true
                        Layout.fillWidth: true
                    }

                }

            }

        }

        Component {
            id: projectSelectorPage

            Page {
                title: qsTr("Open a Project")

                Component {
                    id: projectDelegate

                    Rectangle {
                        id: projectItem

                        required property string path
                        readonly property ListView lv: ListView.view
                        property bool isCurrent: ListView.isCurrentItem

                        implicitHeight: projectTxt.implicitHeight + 2 * 2
                        implicitWidth: lv.width
                        color: palette.base
                        radius: 8
                        border.color: palette.text
                        border.width: 1
                        clip: true

                        Text {
                            id: projectTxt

                            text: projectItem.path
                            horizontalAlignment: Qt.AlignHCenter
                            color: palette.text

                            anchors {
                                left: parent.left
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                                leftMargin: 2
                                rightMargin: 2
                            }

                        }

                    }

                }

                ListView {
                    id: recentProjectsListView

                    function clearRecentProjects() {
                        model.clearRecentProjects();
                    }

                    anchors.fill: parent
                    delegate: projectDelegate
                    model: root.projectManager().recentProjects
                }

                header: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        Label {
                            text: qsTr("Open a Project")
                            elide: Text.ElideRight
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }

                        ToolButton {
                            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "settings-symbolic.svg")
                            onClicked: menu.open()

                            Menu {
                                id: menu

                                Action {
                                    text: qsTr("Device Selector")
                                    onTriggered: {
                                      deviceManager().showDeviceSelector()
                                    }
                                }

                                MenuItem {
                                    // Handle about action

                                    text: qsTr("About Zrythm")
                                    onTriggered: {
                                    }
                                }

                            }

                        }

                    }

                }

                footer: DialogButtonBox {
                    horizontalPadding: 10

                    Button {
                        onClicked: stack.push(createProjectPage)
                        text: qsTr("Create New Project...")
                    }

                    Button {
                        text: qsTr("Open From Path...")
                    }

                }

            }

        }

        Component {
            id: createProjectPage

            Page {
                title: qsTr("Create New Project")

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    TextField {
                        id: projectNameField

                        Layout.fillWidth: true
                        placeholderText: qsTr("Project Name")
                        text: qsTr("Untitled Project")
                    }

                    Binding {
                        target: projectNameField
                        property: "text"
                        value: root.projectManager().getNextAvailableProjectName(projectDirectoryField.selectedUrl, projectNameField.text)
                        when: projectDirectoryField.selectedUrl.toString().length > 0
                    }

                    ZrythmFilePicker {
                        id: projectDirectoryField

                        Layout.fillWidth: true
                        // placeholderText: qsTr("Parent Directory")
                        initialPath: root.settingsManager().new_project_directory
                    }

                    ComboBox {
                        id: projectTemplateField

                        Layout.fillWidth: true
                        textRole: "name"
                        valueRole: "path"

                        model: ProjectTemplatesModel {
                        }

                    }

                    Button {
                        text: qsTr("Create Project")
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: {
                            // start the project creation process asynchronusly
                            root.projectManager().createNewProject(projectDirectoryField.selectedUrl, projectNameField.text, projectTemplateField.currentValue);
                            stack.push(projectCreationProgressPage);
                        }
                    }

                }

            }

        }

        Component {
            id: projectCreationProgressPage

            Page {
                title: qsTr("Creating Project")

                ColumnLayout {
                    spacing: 10

                    anchors {
                        centerIn: parent
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Creating Project...")
                        font.pointSize: 16
                        font.bold: true
                    }

                    BusyIndicator {
                        Layout.alignment: Qt.AlignHCenter
                        running: true
                    }

                }

            }

        }

        Connections {
            function onProjectLoaded(project) {
                console.log("Project loaded: ", project.title);
                root.projectManager().activeProject = project;
                console.log("Opening project: ", root.projectManager().activeProject.title);
                openProjectWindow(project);
            }

            function onProjectLoadingFailed(errorMessage) {
                console.log("Project loading failed: ", errorMessage);
                stack.pop();
                root.alertManager().showAlert(qsTr("Project Loading Failed"), errorMessage);
            }

            target: root.projectManager()
        }

        Connections {
            function onAlertRequested(title, message) {
                console.log("Alert requested: ", title, message);
                alertDialog.alertTitle = title;
                alertDialog.alertMessage = message;
                alertDialog.open();
            }

            target: root.alertManager()
        }

        ZrythmAlertDialog {
            id: alertDialog

            anchors.centerIn: parent
        }

        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 200
            }

        }

        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 200
            }

        }

    }

}
