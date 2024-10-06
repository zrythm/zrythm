// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

ApplicationWindow {
  id: mainWindow
  title: qsTr("Zrythm")
  visible: true
    
  onClosing: {
      // Equivalent to destroy => $on_main_window_destroy();
      // Call the appropriate function here
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
                  id: startDockSwitcher
                  // Implement PanelToggleButton functionality
                  text: qsTr("Toggle Left Panel")
                  onClicked: {
                      // Toggle left panel
                  }
              }

              Rectangle {
                  width: 1
                  height: parent.height
                  color: "gray"
              }

              Button {
                  id: undoBtn
                  text: qsTr("Undo")
                  icon.name: "edit-undo"
                  enabled: false
                  onClicked: {
                      // Perform undo action
                  }
              }

              Button {
                  id: redoBtn
                  text: qsTr("Redo")
                  icon.name: "edit-redo"
                  enabled: false
                  onClicked: {
                      // Perform redo action
                  }
              }

              Rectangle {
                  width: 1
                  height: parent.height
                  color: "gray"
              }

              // Implement ToolboxWidget
              Rectangle {
                  id: toolbox
                  // Add ToolboxWidget properties and functionality
              }
          }

          Label {
              id: windowTitle
              Layout.fillWidth: true
              horizontalAlignment: Text.AlignHCenter
              text: mainWindow.title
          }

          RowLayout {
              id: headerEndBox

              ToolButton {
                  id: endDockSwitcher
                  // Implement PanelToggleButton functionality
                  text: qsTr("Toggle Right Panel")
                  onClicked: {
                      // Toggle right panel
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
                          text: qsTr("Open a Project…")
                          onTriggered: {
                              // Implement open action
                          }
                      }
                      MenuItem {
                          text: qsTr("Create New Project…")
                          onTriggered: {
                              // Implement new project action
                          }
                      }
                      MenuSeparator { }
                      MenuItem {
                          text: qsTr("Save")
                          onTriggered: {
                              // Implement save action
                          }
                      }
                      MenuItem {
                          text: qsTr("Save As…")
                          onTriggered: {
                              // Implement save as action
                          }
                      }
                      MenuSeparator { }
                      MenuItem {
                          text: qsTr("Export As…")
                          onTriggered: {
                              // Implement export as action
                          }
                      }
                      MenuItem {
                          text: qsTr("Export Graph…")
                          onTriggered: {
                              // Implement export graph action
                          }
                      }
                      MenuSeparator { }
                      MenuItem {
                          text: qsTr("Fullscreen")
                          onTriggered: {
                              mainWindow.visibility = mainWindow.visibility === Window.FullScreen ? Window.AutomaticVisibility : Window.FullScreen
                          }
                      }
                      MenuSeparator { }
                      MenuItem {
                          text: qsTr("Preferences")
                          onTriggered: {
                              // Open preferences dialog
                          }
                      }
                      MenuItem {
                          text: qsTr("Keyboard Shortcuts")
                          onTriggered: {
                              // Show keyboard shortcuts
                          }
                      }
                      MenuItem {
                          text: qsTr("About Zrythm")
                          onTriggered: {
                              // Show about dialog
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
              id: centerDock
              anchors.fill: parent
              // Add CenterDockWidget properties and functionality
          }

          // Implement ToastOverlay
          Rectangle {
              id: toastOverlay
              anchors.fill: parent
              // Add ToastOverlay properties and functionality
          }
      }

      // Implement BotBarWidget
      Rectangle {
          id: botBar
          Layout.fillWidth: true
          // Add BotBarWidget properties and functionality
      }
  }

Component.onCompleted: {
    console.log("ApplicationWindow created")
}
}
