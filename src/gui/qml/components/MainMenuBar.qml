// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import "../config.js" as Config
import QtQuick
import QtQuick.Controls
import Zrythm 1.0
import ZrythmStyle 1.0

MenuBar {
  id: root

  required property var deviceManager
  required property var project

  Menu {
    title: qsTr("&File")

    Action {
      text: qsTr("New Long Long Long Long Long Long Long Long Long Name")
    }

    Action {
      text: qsTr("Open")
    }
  }

  Menu {
    title: qsTr("&Edit")

    Action {
      text: qsTr("Undo")
    }
  }

  Menu {
    title: qsTr("&View")

    Menu {
      title: qsTr("Language")

      MenuItem {
        id: systemLocaleMenuItem

        checkable: true
        checked: GlobalState.settingsManager.uiLocale === ""
        text: qsTr("System")

        onTriggered: {
          GlobalState.translationManager.loadTranslation("");
        }
      }

      MenuSeparator {
      }

      Repeater {
        model: Object.keys(Config.languageMap).map(code => {
          return ({
              "code": code,
              "name": Config.languageMap[code]
            });
        })

        delegate: MenuItem {
          id: localeMenuItem

          required property string code
          required property string name

          checkable: true
          checked: GlobalState.settingsManager.uiLocale === code
          text: name

          onTriggered: {
            GlobalState.translationManager.loadTranslation(code);
          }
        }
      }
    }

    Menu {
      title: qsTr("Appearance")

      Action {
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dark-mode-symbolic.svg")
        text: qsTr("Switch Light/Dark Theme")

        onTriggered: {
          Style.darkMode = !Style.darkMode;
        }
      }

      Menu {
        title: qsTr("Theme Color")

        Action {
          enabled: Style.darkMode
          text: qsTr("Zrythm Orange")

          onTriggered: {
            Style.primaryColor = Style.zrythmColor;
          }
        }

        Action {
          text: qsTr("Celestial Blue")

          onTriggered: {
            Style.primaryColor = Style.celestialBlueColor;
          }
        }

        Action {
          enabled: Style.darkMode
          text: qsTr("Jonquil Yellow")

          onTriggered: {
            Style.primaryColor = Style.jonquilYellowColor;
          }
        }

        Action {
          enabled: Style.darkMode
          text: qsTr("Spring Green")

          onTriggered: {
            Style.primaryColor = Style.springGreen;
          }
        }

        Action {
          enabled: Style.darkMode
          text: qsTr("Munsell Red")

          onTriggered: {
            Style.primaryColor = Style.munsellRed;
          }
        }

        Action {
          enabled: !Style.darkMode
          text: qsTr("Gunmetal")

          onTriggered: {
            Style.primaryColor = Style.gunmetalColor;
          }
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

      onTriggered: {}
    }
  }
}
