// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import "../config.js" as Config
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ApplicationWindow {
  id: root

  property Arranger activeArranger: null
  readonly property Action deleteAction: Action {
    id: deleteAction

    enabled: root.activeArranger !== null
    shortcut: StandardKey.Delete
    text: qsTr("&Delete")

    onTriggered: {
      if (root.activeArranger) {
        root.activeArranger.selectionOperator.deleteObjects(root.project.tracklist);
      }
    }
  }
  required property DeviceManager deviceManager
  required property Project project
  required property SettingsManager settingsManager

  function closeAndDestroy() {
    console.log("Closing and destroying project window");
    close();
    destroy();
  }

  height: 720
  title: project.title
  visible: true
  width: 1280

  header: MainToolbar {
    id: headerBar

    project: root.project
    settingsManager: root.settingsManager
  }
  menuBar: MainMenuBar {
    id: mainMenuBar

    aboutDialog: aboutDialog
    deviceManager: root.deviceManager
    project: root.project
  }

  Component.onCompleted: {
    console.log("ApplicationWindow created on platform", Qt.platform.os);
    project.aboutToBeDeleted.connect(closeAndDestroy);
    project.activate();
  }
  onClosing: {
    console.log("Application shutting down");
    closeAndDestroy();
    Qt.quit();
  }

  AboutDialog {
    id: aboutDialog

  }

  Shortcut {
    context: Qt.ApplicationShortcut
    enabled: root.project.undoStack && root.project.undoStack.canUndo
    sequences: [StandardKey.Undo]

    onActivated: root.project.undoStack.undo()
  }

  Shortcut {
    context: Qt.ApplicationShortcut
    enabled: root.project.undoStack && root.project.undoStack.canRedo
    sequences: [StandardKey.Redo]

    onActivated: root.project.undoStack.redo()
  }

  // Global spacebar for play/pause
  Shortcut {
    context: Qt.ApplicationShortcut
    sequence: "Space"

    onActivated: {
      // Toggle play/pause regardless of focus
      if (root.project.transport.isRolling()) {
        root.project.transport.requestPause();
      } else {
        root.project.transport.requestRoll();
      }
    }
  }

  Shortcut {
    context: Qt.ApplicationShortcut
    sequences: [StandardKey.Save]

    onActivated: {
      console.log("save requested");
      // TODO
      // root.project.save();
    }
  }

  // Tools
  Shortcut {
    sequence: "1"

    onActivated: {
      root.project.tool.toolValue = Tool.Select;
    }
  }

  Shortcut {
    sequence: "2"

    onActivated: {
      root.project.tool.toolValue = Tool.Edit;
    }
  }

  Shortcut {
    sequence: "3"

    onActivated: {
      root.project.tool.toolValue = Tool.Cut;
    }
  }

  Shortcut {
    sequence: "4"

    onActivated: {
      root.project.tool.toolValue = Tool.Eraser;
    }
  }

  Shortcut {
    sequence: "5"

    onActivated: {
      root.project.tool.toolValue = Tool.Ramp;
    }
  }

  Shortcut {
    sequence: "6"

    onActivated: {
      root.project.tool.toolValue = Tool.Audition;
    }
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    SplitView {
      id: mainSplitView

      Layout.fillHeight: true
      Layout.fillWidth: true
      orientation: Qt.Horizontal

      LeftDock {
        SplitView.fillHeight: true
        SplitView.minimumWidth: 40
        SplitView.preferredWidth: 200
        project: root.project
        tracklist: root.project.tracklist
        undoStack: root.project.undoStack
        visible: root.settingsManager.leftPanelVisible
      }

      SplitView {
        id: centerSplitView

        SplitView.fillHeight: true
        SplitView.fillWidth: true
        SplitView.minimumWidth: 120
        orientation: Qt.Vertical

        CenterDock {
          Layout.verticalStretchFactor: 2
          SplitView.fillHeight: true
          SplitView.fillWidth: true
          SplitView.minimumHeight: implicitHeight
          SplitView.preferredHeight: 200
          project: root.project
        }

        BottomDock {
          Layout.verticalStretchFactor: 1
          SplitView.fillHeight: true
          SplitView.fillWidth: true
          SplitView.minimumHeight: 40
          SplitView.preferredHeight: 240
          project: root.project
          visible: root.settingsManager.bottomPanelVisible
        }
      }

      RightDock {
        id: rightDock

        SplitView.fillHeight: true
        SplitView.minimumWidth: 30
        SplitView.preferredWidth: 200
        project: root.project
        visible: root.settingsManager.rightPanelVisible
      }
    }

    Item {
      id: botBar

      Layout.fillWidth: true
      implicitHeight: 24

      Text {
        anchors.right: parent.right
        font: root.font
        text: "Status Bar"
      }
    }
  }
}
