// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Zrythm
import ZrythmStyle
import "config.js" as Config

ApplicationWindow {
  id: root

  readonly property ZrythmApplication app: GlobalState.application
  readonly property AppSettings appSettings: app?.appSettings ?? null
  property ProjectWindow currentProjectWindow: null
  readonly property DeviceManager deviceManager: app?.deviceManager ?? null
  readonly property NotificationCenter notificationCenter: app?.notificationCenter ?? null
  readonly property string pendingProjectPath: app?.pendingProjectFile ?? ""
  readonly property PluginManager pluginManager: app?.pluginManager ?? null
  readonly property PluginScanManager pluginScanner: pluginManager?.scanner ?? null
  readonly property ProjectManager projectManager: app?.projectManager ?? null
  property bool pluginScanRunning: false

  function openProjectWindow(session) {
    let newWindow = projectWindowComponent.createObject(session, {
      "session": session,
      "deviceManager": deviceManager,
      "appSettings": appSettings,
      "chordPresetManager": app.chordPresetManager,
      "controlRoom": app.controlRoom,
      "notificationCenter": notificationCenter
    }) as ProjectWindow;
    newWindow.closing.connect(function () {
      root.show();
      root.currentProjectWindow = null;
    });
    newWindow.show();
    currentProjectWindow = newWindow;
    stack.push(projectSelectorPage);
    root.hide();
  }

  function showAudioDeviceHintIfMissing() {
    if (root.deviceManager === null || root.notificationCenter === null)
      return;

    if (root.deviceManager.currentAudioDeviceName !== "")
      return;

    root.notificationCenter.postWarning(qsTr("No Audio Device"), qsTr("Zrythm could not open an audio device. Playback and recording will be silent until one is selected."), "first-run-audio-device", qsTr("Open Device Selector"), () => root.deviceManager.showDeviceSelector());
  }

  font.family: ZrythmTheme.fontFamily
  font.pointSize: ZrythmTheme.fontPointSize
  height: 420
  minimumWidth: 256
  title: "Zrythm"
  visible: true
  width: 640

  onCurrentProjectWindowChanged: {
    // Trigger project cleanup when the project window closes
    if (currentProjectWindow === null) {
      Qt.callLater(function () {
        projectManager.activeSession = null;
      });
    }
  }

  Component.onCompleted: {
    // the scan runs in the background; the selector page shows progress
    if (root.pluginManager !== null) {
      root.pluginManager.beginScan();
      root.pluginScanRunning = true;
    }
  }

  Connections {
    function onScanningFinished() {
      root.pluginScanRunning = false;
    }

    target: root.pluginScanner
  }

  AboutDialog {
    id: aboutDialog
  }

  NotificationArea {
    notificationCenter: root.notificationCenter

    onCriticalNotification: notification => {
      alertDialog.text = notification.title;
      alertDialog.informativeText = notification.detail;
      alertDialog.open();
    }
  }

  LoadController {
    id: loadController
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
    id: finishPage

    PlaceholderPage {
      title: qsTr("All Ready!")

      action: Action {
        text: qsTr("Get Started")

        onTriggered: {
          root.appSettings.firstRun = false;
          root.showAudioDeviceHintIfMissing();
          stack.push(projectSelectorPage);
        }
      }
    }
  }

  StackView {
    id: stack

    anchors.fill: parent
    initialItem: (root.appSettings?.firstRun && root.pendingProjectPath.length === 0) ? firstRunPage : projectSelectorPage

    popEnter: FadeInTransition {
    }
    popExit: FadeOutTransition {
    }
    pushEnter: FadeInTransition {
    }
    pushExit: FadeOutTransition {
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

            addItem(finishPage);
          }
          onCurrentIndexChanged: indicator.currentIndex = currentIndex

          Item {
            PlaceholderPage {
              description: qsTr("Welcome to the Zrythm digital audio workstation. Move to the next page to get started.")
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "zrythm.svg")
              tintIcon: false
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

          Accessible.name: qsTr("Previous Page")
          anchors.left: parent.left
          anchors.leftMargin: 12
          anchors.verticalCenter: parent.verticalCenter
          display: AbstractButton.IconOnly
          height: squareSize
          icon.source: ResourceManager.getIconUrl("gnome-icon-library", "left-large-symbolic.svg")
          visible: welcomeCarousel.currentIndex > 0
          width: squareSize

          onClicked: welcomeCarousel.decrementCurrentIndex()
        }

        // Add right navigation button
        RoundButton {
          Accessible.name: qsTr("Next Page")
          anchors.right: parent.right
          anchors.rightMargin: 12
          anchors.verticalCenter: parent.verticalCenter
          display: AbstractButton.IconOnly
          height: leftNavButton.squareSize
          icon.source: ResourceManager.getIconUrl("gnome-icon-library", "right-large-symbolic.svg")
          visible: welcomeCarousel.currentIndex < welcomeCarousel.count - 1
          width: leftNavButton.squareSize

          onClicked: welcomeCarousel.incrementCurrentIndex()
        }

        PageIndicator {
          id: indicator

          anchors.bottom: welcomeCarousel.bottom
          anchors.bottomMargin: 12
          anchors.horizontalCenter: parent.horizontalCenter
          count: welcomeCarousel.count
          currentIndex: welcomeCarousel.currentIndex
          interactive: true

          onCurrentIndexChanged: welcomeCarousel.currentIndex = currentIndex
        }
      }
    }

    Component {
      id: projectSelectorPage

      NavigatablePage {
        // Distinguishes this page when inspecting the StackView's current item
        readonly property bool isProjectSelectorPage: true
        property bool pendingProjectLoadHandled: false

        notificationCenter: root.notificationCenter
        showBackButton: false
        title: qsTr("Open a Project")

        StackView.onActivated: {
          // open the project requested on the command line or via a
          // file-open request from the OS (once)
          if (!pendingProjectLoadHandled && root.pendingProjectPath.length > 0) {
            pendingProjectLoadHandled = true;
            loadController.loadProject(root.pendingProjectPath);
          }
        }

        resources: [
          Action {
            id: clearRecentProjectsAction

            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "edit-delete-symbolic.svg")
            text: qsTr("Clear Recent Projects")

            onTriggered: clearRecentProjectsDialog.open()
          }
        ]

        footer: DialogButtonBox {
          horizontalPadding: 10

          Button {
            text: qsTr("Create New Project...")

            onClicked: stack.push(createProjectPageComponent)
          }

          Button {
            text: qsTr("Open From Path...")

            onClicked: loadController.openLoadDialog()
          }
        }
        rightActions: ToolButton {
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

        ColumnLayout {
          anchors.fill: parent
          anchors.margins: 16
          spacing: ZrythmTheme.buttonPadding

          ActionRowGroup {
            Layout.fillHeight: true
            Layout.fillWidth: true
            suffixActions: [clearRecentProjectsAction]
            title: qsTr("Recent Projects")
            visible: recentProjectsListView.count > 0

            ListView {
              id: recentProjectsListView

              readonly property RecentProjectsModel recentProjects: root.projectManager?.recentProjects

              function formatLastSavedDate(lastSaved: date): string {
                if (!(lastSaved instanceof Date) || isNaN(lastSaved.getTime()))
                  return "";

                const minutes = Math.floor((Date.now() - lastSaved.getTime()) / 60000);
                if (minutes < 1)
                  return qsTr("Just now");
                if (minutes < 60)
                  return qsTr("%n minute(s) ago", "", minutes);
                const hours = Math.floor(minutes / 60);
                if (hours < 24)
                  return qsTr("%n hour(s) ago", "", hours);
                const days = Math.floor(hours / 24);
                if (days === 1)
                  return qsTr("Yesterday");
                if (days < 7)
                  return qsTr("%n day(s) ago", "", days);
                return lastSaved.toLocaleString(Qt.locale(), Locale.ShortFormat);
              }

              clip: true
              Layout.fillHeight: true
              Layout.fillWidth: true
              model: recentProjects

              delegate: ActionRow {
                id: projectItem

                required property date date
                required property string name
                required property string path

                hoverBaseColor: ZrythmTheme.cardColor
                interactive: true
                overflowActions: [revealAction]
                subtitle: projectItem.path
                suffixActions: [removeAction]
                title: projectItem.name
                width: recentProjectsListView.width

                resources: [
                  Action {
                    id: revealAction

                    text: qsTr("Show in File Manager")

                    onTriggered: QmlUtils.revealInFileManager(projectItem.path)
                  },
                  Action {
                    id: removeAction

                    icon.source: ResourceManager.getIconUrl("gnome-icon-library", "edit-delete-symbolic.svg")
                    text: qsTr("Remove From List")

                    onTriggered: recentProjectsListView.recentProjects.removeRecentProject(projectItem.path)
                  }
                ]

                onClicked: loadController.loadProject(projectItem.path)

                Label {
                  color: palette.text
                  font: ZrythmTheme.smallTextFont
                  opacity: 0.55
                  text: recentProjectsListView.formatLastSavedDate(projectItem.date)
                  visible: text !== ""
                }
              }

              Component.onCompleted: {
                recentProjects.clearNonExistingProjects();
              }
            }
          }

          Label {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: palette.text
            horizontalAlignment: Text.AlignHCenter
            opacity: 0.6
            text: qsTr("No recent projects yet.\nCreate or open a project to see it here.")
            verticalAlignment: Text.AlignVCenter
            visible: recentProjectsListView.count === 0
            wrapMode: Text.WordWrap
          }

          RowLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: root.pluginScanRunning

            BusyIndicator {
              Layout.preferredHeight: 16
              Layout.preferredWidth: 16
              running: root.pluginScanRunning
            }

            Label {
              color: palette.text
              font: ZrythmTheme.smallTextFont
              opacity: 0.55
              text: qsTr("Scanning Plugins...")
            }
          }
        }

        Dialog {
          id: clearRecentProjectsDialog

          contentItem: ColumnLayout {
            Label {
              Layout.fillWidth: true
              text: qsTr("All projects will be removed from the list. Files on disk are not deleted.")
              wrapMode: Text.WordWrap
            }
          }
          modal: true
          title: qsTr("Clear Recent Projects?")

          footer: DialogButtonBox {
            standardButtons: DialogButtonBox.Cancel

            DestructiveButton {
              DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
              text: qsTr("Clear")
            }
          }

          onAccepted: recentProjectsListView.recentProjects.clearRecentProjects()
        }
      }
    }

    Component {
      id: createProjectPageComponent

      NavigatablePage {
        id: createProjectPage

        notificationCenter: root.notificationCenter
        title: qsTr("Create New Project")

        footer: DialogButtonBox {
          defaultButton: createProjectButton
          horizontalPadding: 10
          standardButtons: DialogButtonBox.Cancel

          onRejected: createProjectPage.StackView.view.pop()

          Button {
            id: createProjectButton

            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            enabled: projectNameField.text.length > 0 && projectDirectoryField.selectedUrl.toString().length > 0
            text: qsTr("Create Project")

            onClicked: {
              // remember the chosen directory as the default for the next new project
              if (root.appSettings !== null)
                root.appSettings.newProjectDirectory = QmlUtils.toPathString(projectDirectoryField.selectedUrl);

              // start the project creation process asynchronously
              root.projectManager.createNewProject(projectDirectoryField.selectedUrl, projectNameField.text, projectTemplateField.currentValue);
              stack.push(projectCreationProgressPage);
            }
          }
        }

        ColumnLayout {
          anchors {
            left: parent.left
            margins: 16
            right: parent.right
            top: parent.top
          }
          spacing: ZrythmTheme.buttonPadding

          Binding {
            property: "text"
            target: projectNameField
            value: root.projectManager?.getNextAvailableProjectName(projectDirectoryField.selectedUrl, projectNameField.text)
            when: projectDirectoryField.selectedUrl.toString().length > 0
          }

          ActionRowGroup {
            Layout.fillWidth: true
            title: qsTr("Project Details")

            ActionRow {
              subtitle: qsTr("Also used as the folder name")
              title: qsTr("Name")

              TextField {
                id: projectNameField

                Layout.preferredWidth: 240
                placeholderText: qsTr("Project Name")
                text: qsTr("Untitled Project")
              }
            }

            ActionRow {
              subtitle: qsTr("Where the project folder will be created")
              title: qsTr("Location")

              ZrythmFilePicker {
                id: projectDirectoryField

                Layout.preferredWidth: 260
                defaultPath: root.appSettings?.newProjectDirectoryDefaultValue() ?? ""
                initialPath: root.appSettings?.newProjectDirectory ?? ""
              }
            }

            ActionRow {
              subtitle: qsTr("Starting contents for the new project")
              title: qsTr("Template")

              ComboBox {
                id: projectTemplateField

                Layout.preferredWidth: 200
                textRole: "name"
                valueRole: "path"

                model: ProjectTemplatesModel {
                }
              }
            }
          }
        }
      }
    }

    Component {
      id: projectCreationProgressPage

      NavigatablePage {
        notificationCenter: root.notificationCenter
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
      function onProjectLoaded(session: ProjectSession) {
        // ProjectManager already set the active session and updated the
        // recent projects list before emitting this signal
        console.log("Project loaded: ", session.title);
        root.appSettings.firstRun = false;
        root.openProjectWindow(session);
      }

      function onProjectLoadingFailed(errorMessage: string) {
        console.log("Project loading failed: ", errorMessage);
        if (!stack.currentItem?.isProjectSelectorPage) {
          stack.push(projectSelectorPage);
        }
      }

      target: root.projectManager
    }

    MessageDialog {
      id: alertDialog

      buttons: MessageDialog.Ok
    }
  }

  component FadeInTransition: Transition {
    PropertyAnimation {
      duration: ZrythmTheme.animationDuration
      from: 0
      property: "opacity"
      to: 1
    }
  }
  component FadeOutTransition: Transition {
    PropertyAnimation {
      duration: ZrythmTheme.animationDuration
      from: 1
      property: "opacity"
      to: 0
    }
  }

  // Inline component for pages with standard navigation header
  component NavigatablePage: Page {
    id: navigatablePage

    // Public API
    property NotificationCenter notificationCenter: null
    property alias rightActions: rightActionsRow.data
    property bool showBackButton: navigatablePage.StackView.view && navigatablePage.StackView.view.depth > 1

    header: ToolBar {
      ToolButton {
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        text: "‹"
        visible: navigatablePage.showBackButton

        onClicked: navigatablePage.StackView.view.pop()
      }

      // Centered on the whole header so asymmetric side controls don't
      // shift the title
      Label {
        anchors.centerIn: parent
        elide: Text.ElideRight
        font.bold: true
        text: navigatablePage.title
        width: Math.min(implicitWidth, parent.width - 128)
      }

      RowLayout {
        id: rightActionsRow

        anchors.right: notificationCenterButton.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4
      }

      NotificationCenterButton {
        id: notificationCenterButton

        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        notificationCenter: navigatablePage.notificationCenter
      }
    }
  }
}
