// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

ZrythmToolBar {
  id: root

  required property Project project
  required property SettingsManager settingsManager

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
      checked: root.settingsManager.leftPanelVisible
      icon.source: Qt.resolvedUrl("../icons/gnome-icon-library/dock-left-symbolic.svg")

      onCheckedChanged: {
          root.settingsManager.leftPanelVisible = checked;
      }

      ToolTip {
        text: qsTr("Toggle Left Panel")
      }

      Connections {
        function onRightPanelVisibleChanged() {
            toggleLeftDock.checked = root.settingsManager.leftPanelVisible;
        }

        target: root.settingsManager
      }
    },
    ToolSeparator {
    },
    UndoSplitButton {
      id: undoBtn

      isUndo: true
      undoStack: root.project.undoStack
    },
    UndoSplitButton {
      id: redoBtn

      isUndo: false
      undoStack: root.project.undoStack
    },
    ToolBox {
      id: toolBox

      tool: root.project.tool
    }
  ]
  rightItems: [
    SpectrumAnalyzer {
      leftPort: root.project.tracklist.singletonTracks.masterTrack.channel.leftAudioOut
      padding: 2
      rightPort: root.project.tracklist.singletonTracks.masterTrack.channel.rightAudioOut
      width: 120
    },
    WaveformViewer {
      leftPort: root.project.tracklist.singletonTracks.masterTrack.channel.leftAudioOut
      padding: 2
      rightPort: root.project.tracklist.singletonTracks.masterTrack.channel.rightAudioOut
      width: 120
    },
    DspLoadIndicator {
      id: dspLoadIndicator
    },
    ToolButton {
      id: toggleRightDock

      checkable: true
      checked: root.settingsManager.rightPanelVisible
      icon.source: Qt.resolvedUrl("../icons/gnome-icon-library/dock-right-symbolic.svg")

      onCheckedChanged: {
          root.settingsManager.rightPanelVisible = checked;
      }

      Connections {
        function onRightPanelVisibleChanged() {
            toggleRightDock.checked = root.settingsManager.rightPanelVisible;
        }

        target: root.settingsManager
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

  Timer {
    interval: 600
    repeat: true
    running: true

    onTriggered: {
      dspLoadIndicator.xRuns = root.project.engine.xRunCount();
      dspLoadIndicator.loadValue = root.project.engine.loadPercentage();
    }
  }
}
