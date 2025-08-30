// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import "../config.js" as Config
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ApplicationWindow {
  id: root

  required property DeviceManager deviceManager
  required property Project project

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
  }
  menuBar: MainMenuBar {
    id: mainMenuBar

    deviceManager: root.deviceManager
    project: root.project
  }

  Component.onCompleted: {
    console.log("ApplicationWindow created on platform", Qt.platform.os);
    project.aboutToBeDeleted.connect(closeAndDestroy);
    project.activate();
  }
  onClosing: {}

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
        root.project.transport.requestPause(true);
      } else {
        root.project.transport.requestRoll(true);
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
        visible: GlobalState.settingsManager.leftPanelVisible
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
          visible: GlobalState.settingsManager.bottomPanelVisible
        }
      }

      RightDock {
        id: rightDock

        SplitView.fillHeight: true
        SplitView.minimumWidth: 30
        SplitView.preferredWidth: 200
        visible: GlobalState.settingsManager.rightPanelVisible
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
