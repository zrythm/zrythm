// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle
import "config.js" as Config

ApplicationWindow {
  id: root

  readonly property AlertManager alertManager: app?.alertManager ?? null
  readonly property ZrythmApplication app: GlobalState.application
  readonly property DeviceManager deviceManager: app?.deviceManager ?? null
  readonly property PluginManager pluginManager: app?.pluginManager ?? null
  readonly property PluginScanManager pluginScanner: pluginManager?.scanner ?? null
  readonly property ProjectManager projectManager: app?.projectManager ?? null
  readonly property AppSettings appSettings: app?.appSettings ?? null

  function openProjectWindow(projectUiState) {
    let newWindow = projectWindowComponent.createObject(projectUiState, {
      "projectUiState": projectUiState,
      "deviceManager": deviceManager,
      "appSettings": appSettings,
      "controlRoom": app.controlRoom
    }) as ProjectWindow;
    newWindow.show();
    root.close();
  }

  font.family: Style.fontFamily
  font.pointSize: Style.fontPointSize
  height: 420
  minimumWidth: 256
  title: "Zrythm"
  visible: true
  width: 640

  AboutDialog {
    id: aboutDialog

  }

  Component {
    id: projectWindowComponent

    ProjectWindow {
    }
  }

  Item {
    id: flatpakPage

    PlaceholderPage {
      description: qsTr("Only audio plugins installed via Flatpak are supported.")
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "flatpak-symbolic.svg")
      title: qsTr("About Flatpak")
    }
  }

  Item {
    id: donationPage

    PlaceholderPage {
      description: qsTr("Zrythm relies on donations and purchases to sustain development. If you enjoy the software, please consider %1donating%2 or %3buying an installer%2.").arg("<a href=\"" + Config.DONATION_URL + "\">").arg("</a>").arg("<a href=\"" + Config.PURCHASE_URL + "\">")
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "credit-card-symbolic.svg")
      title: qsTr("Donate")
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
    initialItem: root.appSettings?.first_run ? firstRunPage : progressPage

    popExit: Transition {
      PropertyAnimation {
        duration: 200
        from: 1
        property: "opacity"
        to: 0
      }
    }
    pushEnter: Transition {
      PropertyAnimation {
        duration: 200
        from: 0
        property: "opacity"
        to: 1
      }
    }

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
              description: qsTr("Welcome to the Zrythm digital audio workstation. Move to the next page to get started.")
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "zrythm.svg")
              title: qsTr("Welcome")
            }
          }

          Item {
            PlaceholderPage {
              description: qsTr("If this is your first time using Zrythm, we suggest going through the 'Getting Started' section in the %1user manual%2.").arg("<a href=\"" + Config.USER_MANUAL_URL + "\">").arg("</a>")
              icon.source: ResourceManager.getIconUrl("gnome-icon-library", "open-book-symbolic.svg")
              title: qsTr("Read the Manual")
            }
          }
        }

        // Add left navigation button
        RoundButton {
          id: leftNavButton

          readonly property real squareSize: 48

          anchors.left: parent.left
          anchors.verticalCenter: parent.verticalCenter
          font.bold: true
          font.pixelSize: 18
          height: squareSize
          text: "<"
          visible: welcomeCarousel.currentIndex > 0
          width: squareSize

          onClicked: welcomeCarousel.decrementCurrentIndex()
        }

        // Add right navigation button
        RoundButton {
          anchors.right: parent.right
          anchors.verticalCenter: parent.verticalCenter
          font.bold: true
          font.pixelSize: 18
          height: leftNavButton.squareSize
          text: ">"
          visible: welcomeCarousel.currentIndex < welcomeCarousel.count - 1
          width: leftNavButton.squareSize

          onClicked: welcomeCarousel.incrementCurrentIndex()
        }

        PageIndicator {
          id: indicator

          anchors.bottom: welcomeCarousel.bottom
          anchors.horizontalCenter: parent.horizontalCenter
          count: welcomeCarousel.count
          currentIndex: welcomeCarousel.currentIndex
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

        footer: DialogButtonBox {
          horizontalPadding: 10
          standardButtons: DialogButtonBox.Reset

          onReset: {
            console.log("Resetting settings...");
          }

          Button {
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            highlighted: true
            text: qsTr("Continue")

            onClicked: {
              console.log("Proceeding to next page...");
              root.appSettings.first_run = false;
              stack.push(progressPage);
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
              Layout.fillWidth: true
              elide: Text.ElideRight
              font.bold: true
              horizontalAlignment: Qt.AlignHCenter
              text: qsTr("Configuration")
              verticalAlignment: Qt.AlignVCenter
            }
          }
        }

        ZrythmPreferencesPage {
          anchors.fill: parent
          title: qsTr("Initial Configuration")

          ZrythmActionRow {
            subtitle: "Preferred language"
            title: "Language"

            ComboBox {
              model: ["English", "Spanish", "French"]
            }
          }

          ZrythmActionRow {
            subtitle: "Location to save user files"
            title: "User Path"

            ZrythmFilePicker {
            }
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
          // start the scan in the background and move on to project selection page immediately
          root.pluginManager.beginScan();
          stack.push(projectSelectorPage);
        }

        PlaceholderPage {
          icon.source: ResourceManager.getIconUrl("zrythm-dark", "zrythm-monochrome.svg")
          title: qsTr("Scanning Plugins")
        }

        ColumnLayout {
          anchors.bottom: parent.bottom
          anchors.left: parent.left
          anchors.margins: 26
          anchors.right: parent.right
          spacing: 12

          Text {
            id: scanProgressLabel

            Layout.fillWidth: true
            color: palette.text
            font.pointSize: 8
            horizontalAlignment: Qt.AlignHCenter
            opacity: 0.6
            text: qsTr("Scanning:") + " " + root.pluginManager?.currentlyScanningPlugin
            verticalAlignment: Qt.AlignVCenter
          }

          ProgressBar {
            id: scanProgressBar

            Layout.fillWidth: true
            indeterminate: true
          }
        }
      }
    }

    Component {
      id: projectSelectorPage

      Page {
        title: qsTr("Open a Project")

        footer: DialogButtonBox {
          horizontalPadding: 10

          Button {
            text: qsTr("Create New Project...")

            onClicked: stack.push(createProjectPage)
          }

          Button {
            text: qsTr("Open From Path...")
          }
        }
        header: ToolBar {
          RowLayout {
            anchors.fill: parent

            Label {
              Layout.fillWidth: true
              elide: Text.ElideRight
              horizontalAlignment: Qt.AlignHCenter
              text: qsTr("Open a Project")
              verticalAlignment: Qt.AlignVCenter
            }

            ToolButton {
              icon.source: ResourceManager.getIconUrl("gnome-icon-library", "settings-symbolic.svg")

              onClicked: menu.open()

              Menu {
                id: menu

                Action {
                  text: qsTr("Device Selector")

                  onTriggered: {
                    root.deviceManager.showDeviceSelector();
                  }
                }

                MenuItem {
                  // Handle about action

                  text: qsTr("About Zrythm")

                  onTriggered: aboutDialog.open()
                }
              }
            }
          }
        }

        Component {
          id: projectDelegate

          Rectangle {
            id: projectItem

            property bool isCurrent: ListView.isCurrentItem
            readonly property ListView lv: ListView.view
            required property string path

            border.color: palette.text
            border.width: 1
            clip: true
            color: palette.base
            implicitHeight: projectTxt.implicitHeight + 2 * 2
            implicitWidth: lv.width
            radius: 8

            Text {
              id: projectTxt

              color: palette.text
              horizontalAlignment: Qt.AlignHCenter
              text: projectItem.path

              anchors {
                left: parent.left
                leftMargin: 2
                right: parent.right
                rightMargin: 2
                verticalCenter: parent.verticalCenter
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
          model: root.projectManager?.recentProjects
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
            property: "text"
            target: projectNameField
            value: root.projectManager?.getNextAvailableProjectName(projectDirectoryField.selectedUrl, projectNameField.text)
            when: projectDirectoryField.selectedUrl.toString().length > 0
          }

          ZrythmFilePicker {
            id: projectDirectoryField

            Layout.fillWidth: true
            // placeholderText: qsTr("Parent Directory")
            initialPath: root.appSettings?.new_project_directory ?? ""
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
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Create Project")

            onClicked: {
              // start the project creation process asynchronusly
              root.projectManager.createNewProject(projectDirectoryField.selectedUrl, projectNameField.text, projectTemplateField.currentValue);
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
            font.bold: true
            font.pointSize: 16
            text: qsTr("Creating Project...")
          }

          BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: true
          }
        }
      }
    }

    Connections {
      function onProjectLoaded(projectUiState : ProjectUiState) {
        console.log("Project loaded: ", projectUiState.title);
        root.projectManager.activeProject = projectUiState;
        console.log("Opening project: ", root.projectManager.activeProject.title);
        root.openProjectWindow(projectUiState);
      }

      function onProjectLoadingFailed(errorMessage : string) {
        console.log("Project loading failed: ", errorMessage);
        stack.pop();
        root.alertManager.showAlert(qsTr("Project Loading Failed"), errorMessage);
      }

      target: root.projectManager
    }

    Connections {
      function onAlertRequested(title, message) {
        console.log("Alert requested: ", title, message);
        alertDialog.alertTitle = title;
        alertDialog.alertMessage = message;
        alertDialog.open();
      }

      target: root.alertManager
    }

    ZrythmAlertDialog {
      id: alertDialog

      anchors.centerIn: parent
    }
  }
}
