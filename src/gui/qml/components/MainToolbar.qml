// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ZrythmToolBar {
  id: root

  required property var project

  centerItems: [
    TransportControls {
      id: transportControls

      tempoMap: root.project.tempoMap
      transport: root.project.transport
    }
  ]
  leftItems: [
    ToolButton {
      // see https://forum.qt.io/post/529470 for bi-directional binding

      id: toggleLeftDock

      checkable: true
      checked: GlobalState.settingsManager.leftPanelVisible
      icon.source: Qt.resolvedUrl("../icons/gnome-icon-library/dock-left-symbolic.svg")

      onCheckedChanged: {
        GlobalState.settingsManager.leftPanelVisible = checked;
      }

      ToolTip {
        text: qsTr("Toggle Left Panel")
      }

      Connections {
        function onRightPanelVisibleChanged() {
          toggleLeftDock.checked = GlobalState.settingsManager.leftPanelVisible;
        }

        target: GlobalState.settingsManager
      }
    },
    ToolSeparator {
    },
    UndoSplitButton {
      id: undoBtn

      isUndo: true
      undoManager: root.project.undoManager
    },
    UndoSplitButton {
      id: redoBtn

      isUndo: false
      undoManager: root.project.undoManager
    },
    ToolBox {
      id: toolBox

      tool: root.project.tool
    }
  ]
  rightItems: [
    ToolButton {
      id: toggleRightDock

      checkable: true
      checked: GlobalState.settingsManager.rightPanelVisible
      icon.source: Qt.resolvedUrl("../icons/gnome-icon-library/dock-right-symbolic.svg")

      onCheckedChanged: {
        GlobalState.settingsManager.rightPanelVisible = checked;
      }

      Connections {
        function onRightPanelVisibleChanged() {
          toggleRightDock.checked = GlobalState.settingsManager.rightPanelVisible;
        }

        target: GlobalState.settingsManager
      }

      ToolTip {
        text: qsTr("Toggle Right Panel")
      }
    },
    ToolSeparator {
    },
    ToolButton {
      id: menuButton

      text: qsTr("Menu")

      // icon.source: ResourceManager.getIconUrl("gnome-icon-library", "open-menu-symbolic.svg")
      onClicked: primaryMenu.open()

      Menu {
        id: primaryMenu

        Action {
          text: qsTr("Open a Project…")
        }

        MenuItem {
          // Implement new project action

          text: qsTr("Create New Project…")

          onTriggered: {}
        }

        MenuSeparator {
        }

        MenuItem {
          // Implement save action

          text: qsTr("Save")

          onTriggered: {}
        }

        MenuItem {
          // Implement save as action

          text: qsTr("Save As…")

          onTriggered: {}
        }

        MenuSeparator {
        }

        MenuItem {
          // Implement export as action

          text: qsTr("Export As…")

          onTriggered: {}
        }

        MenuItem {
          // Implement export graph action

          text: qsTr("Export Graph…")

          onTriggered: {}
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

          onTriggered: {}
        }

        MenuItem {
          // Show keyboard shortcuts

          text: qsTr("Keyboard Shortcuts")

          onTriggered: {}
        }

        MenuItem {
          // Show about dialog

          text: qsTr("About Zrythm Long Long Long Long Long Long Long")

          onTriggered: {}
        }
      }
    }
  ]
}
