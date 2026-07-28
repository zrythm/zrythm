// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import "../config.js" as Config
import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle
import Qt.labs.synchronizer

MenuBar {
  id: root

  required property AboutDialog aboutDialog
  required property DeviceManager deviceManager
  required property ExportDialog exportDialog
  required property LoadController loadController
  readonly property Project project: session.project
  required property SaveController saveController
  required property ProjectSession session

  Component {
    id: localeMenuItemComponent

    MenuItem {
      required property string code
      required property string displayName

      checkable: true
      checked: GlobalState.application.appSettings.uiLocale === code
      text: displayName

      onTriggered: {
        GlobalState.application.translationManager.loadTranslation(code);
      }
    }
  }

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
      id: languageMenu

      function mainEntries() {
        // keep the curated order of Config.mainLanguageCodes
        return Config.mainLanguageCodes.map(makeEntry);
      }

      function makeEntry(code) {
        const info = Config.languageMap[code];
        // U+200E (LRM) on both sides of the name keeps the percentage suffix at the right end for RTL language names
        return ({
            "code": code,
            "name": info.name,
            "displayName": code === "en" ? info.name : "\u200E" + info.name + "\u200E (" + info.percent + "%)"
          });
      }

      function moreEntries() {
        return Object.keys(Config.languageMap).filter(code => !Config.mainLanguageCodes.includes(code)).map(makeEntry).sort((a, b) => a.code.localeCompare(b.code));
      }

      title: qsTr("Language")

      MenuItem {
        id: systemLocaleMenuItem

        checkable: true
        checked: GlobalState.application.appSettings.uiLocale === ""
        text: qsTr("System")

        onTriggered: {
          GlobalState.application.translationManager.loadTranslation("");
        }
      }

      MenuSeparator {
      }

      Repeater {
        delegate: localeMenuItemComponent
        model: languageMenu.mainEntries()
      }

      MenuSeparator {
      }

      Menu {
        title: qsTr("More Languages")

        Repeater {
          delegate: localeMenuItemComponent
          model: languageMenu.moreEntries()
        }
      }
    }

    Menu {
      title: qsTr("Appearance")

      Action {
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dark-mode-symbolic.svg")
        text: qsTr("Switch Light/Dark Theme")

        onTriggered: ZrythmTheme.toggleDarkMode()
      }

      Menu {
        title: qsTr("Theme Color")

        Action {
          enabled: ZrythmTheme.darkMode
          text: qsTr("Zrythm Orange")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.zrythmColor;
          }
        }

        Action {
          text: qsTr("Celestial Blue")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.celestialBlueColor;
          }
        }

        Action {
          enabled: ZrythmTheme.darkMode
          text: qsTr("Jonquil Yellow")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.jonquilYellowColor;
          }
        }

        Action {
          enabled: ZrythmTheme.darkMode
          text: qsTr("Spring Green")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.springGreen;
          }
        }

        Action {
          enabled: ZrythmTheme.darkMode
          text: qsTr("Munsell Red")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.munsellRed;
          }
        }

        Action {
          enabled: !ZrythmTheme.darkMode
          text: qsTr("Gunmetal")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.gunmetalColor;
          }
        }

        Action {
          text: qsTr("Electric Purple")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.electricPurple;
          }
        }
      }
    }

    MenuItem {
      action: ApplicationWindow.window?.fullScreenAction ?? null
    }

    Menu {
      title: qsTr("Debug")

      MenuItem {
        checkable: true
        text: qsTr("Show Cache Activity")

        Synchronizer on checked {
          sourceObject: GlobalState.application.appSettings
          sourceProperty: "showCacheActivity"
        }
      }
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
