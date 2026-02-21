// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle
import Qt.labs.synchronizer

ZrythmToolBar {
  id: root

  required property AppSettings appSettings
  required property ControlRoom controlRoom
  readonly property Project project: projectUiState.project
  required property ProjectUiState projectUiState

  centerItems: [
    TransportControls {
      id: transportControls

      metronome: root.controlRoom.metronome
      tempoMap: root.project.tempoMap
      transport: root.project.transport
      transportController: root.projectUiState.transportController
    }
  ]
  leftItems: [
    ToolButton {
      id: toggleLeftDock

      checkable: true
      icon.source: Qt.resolvedUrl("../icons/gnome-icon-library/dock-left-symbolic.svg")

      Synchronizer on checked {
        sourceObject: root.appSettings
        sourceProperty: "leftPanelVisible"
      }

      ToolTip {
        text: qsTr("Toggle Left Panel")
      }
    },
    ToolSeparator {
    },
    UndoSplitButton {
      id: undoBtn

      isUndo: true
      undoStack: root.projectUiState.undoStack
    },
    UndoSplitButton {
      id: redoBtn

      isUndo: false
      undoStack: root.projectUiState.undoStack
    },
    ToolBox {
      id: toolBox

      tool: root.projectUiState.tool
    },
    SnapGridButton {
      snapGrid: root.projectUiState.snapGridTimeline
    }
  ]
  rightItems: [
    SpectrumAnalyzer {
      audioEngine: root.project.engine
      padding: 2
      stereoPort: root.project.tracklist.singletonTracks.masterTrack.channel.audioOutPort
      width: 120
    },
    WaveformViewer {
      audioEngine: root.project.engine
      padding: 2
      stereoPort: root.project.tracklist.singletonTracks.masterTrack.channel.audioOutPort
      width: 120
    },
    DspLoadIndicator {
      id: dspLoadIndicator

    },
    ToolButton {
      id: toggleRightDock

      checkable: true
      icon.source: Qt.resolvedUrl("../icons/gnome-icon-library/dock-right-symbolic.svg")

      Synchronizer on checked {
        sourceObject: root.appSettings
        sourceProperty: "rightPanelVisible"
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
