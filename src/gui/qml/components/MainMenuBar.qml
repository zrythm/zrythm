// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Zrythm
import ZrythmDocking
import Qt.labs.synchronizer

MenuBar {
  id: root

  required property AboutDialog aboutDialog
  required property Action copyAction
  required property Action cutAction
  required property Action deleteAction
  required property DeviceManager deviceManager
  required property ProjectDockWorkspace dockWorkspace
  required property Action duplicateAction
  required property ExportDialog exportDialog
  required property FileDialog importFileDialog
  required property LoadController loadController
  required property Action pasteAction
  required property PreferencesDialog preferencesDialog
  readonly property Project project: session.project
  required property SaveController saveController
  required property ProjectSession session

  Menu {
    title: qsTr("&File")

    MenuItem {
      action: root.saveController.saveAction
    }

    MenuItem {
      action: root.saveController.saveAsAction
    }

    MenuItem {
      action: root.loadController.loadAction
    }

    Action {
      text: qsTr("Import File…")

      onTriggered: root.importFileDialog.open()
    }

    Action {
      text: qsTr("Export…")

      onTriggered: {
        root.exportDialog.open();
      }
    }
  }

  Menu {
    title: qsTr("&Edit")

    Action {
      id: undoAction

      enabled: root.session.undoStack && root.session.undoStack.canUndo
      shortcut: StandardKey.Undo
      text: enabled ? "%1: %2".arg(qsTr("Undo")).arg(root.session.undoStack.undoActions[0]) : qsTr("Undo")

      onTriggered: root.session.undoStack.undo()
    }

    Action {
      id: redoAction

      enabled: root.session.undoStack && root.session.undoStack.canRedo
      shortcut: StandardKey.Redo
      text: enabled ? "%1: %2".arg(qsTr("Redo")).arg(root.session.undoStack.redoActions[0]) : qsTr("Redo")

      onTriggered: root.session.undoStack.redo()
    }

    MenuSeparator {
    }

    MenuItem {
      action: root.cutAction
    }

    MenuItem {
      action: root.copyAction
    }

    MenuItem {
      action: root.pasteAction
    }

    MenuItem {
      action: root.duplicateAction
    }

    MenuItem {
      action: root.deleteAction
    }

    MenuSeparator {
    }

    Action {
      shortcut: StandardKey.Preferences
      text: qsTr("Preferences…")

      onTriggered: root.preferencesDialog.open()
    }
  }

  Menu {
    title: qsTr("&View")

    MenuItem {
      checkable: true
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dock-left-symbolic.svg")
      text: qsTr("Left Panel")

      Synchronizer on checked {
        sourceObject: GlobalState.application.appSettings
        sourceProperty: "leftPanelVisible"
      }
    }

    MenuItem {
      checkable: true
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dock-bottom-symbolic.svg")
      text: qsTr("Bottom Panel")

      Synchronizer on checked {
        sourceObject: GlobalState.application.appSettings
        sourceProperty: "bottomPanelVisible"
      }
    }

    MenuItem {
      checkable: true
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dock-right-symbolic.svg")
      text: qsTr("Right Panel")

      Synchronizer on checked {
        sourceObject: GlobalState.application.appSettings
        sourceProperty: "rightPanelVisible"
      }
    }

    MenuSeparator {
    }

    Menu {
      id: panelsMenu

      title: qsTr("Panels")

      Instantiator {
        model: root.dockWorkspace.dockingController.panelIds

        delegate: MenuItem {
          required property string modelData
          readonly property DockPanel panel: root.dockWorkspace.dockingController.panelById(modelData)

          checkable: true
          checked: root.dockWorkspace.dockingController.layout.hidden.indexOf(modelData) < 0
          icon.source: panel.icon
          text: panel.title

          onTriggered: {
            if (root.dockWorkspace.dockingController.isPanelHidden(modelData))
              root.dockWorkspace.dockingController.presentPanel(modelData);
            else
              root.dockWorkspace.dockingController.hidePanel(modelData);
          }
        }

        onObjectAdded: (index, object) => panelsMenu.addItem(object)
        onObjectRemoved: (index, object) => panelsMenu.removeItem(object)
      }
    }

    MenuItem {
      text: qsTr("Save Layout")

      onTriggered: root.dockWorkspace.saveLayout()
    }

    MenuItem {
      text: qsTr("Reset Layout")

      onTriggered: root.dockWorkspace.resetLayout()
    }

    MenuSeparator {
    }

    MenuItem {
      action: ApplicationWindow.window?.fullScreenAction ?? null
    }
  }

  Menu {
    title: qsTr("Devices")

    Action {
      text: qsTr("Audio/MIDI Setup")

      onTriggered: {
        root.deviceManager.showDeviceSelector();
      }
    }
  }

  Menu {
    title: qsTr("&Help")

    Action {
      text: qsTr("About Zrythm")

      onTriggered: root.aboutDialog.open()
    }
  }
}
