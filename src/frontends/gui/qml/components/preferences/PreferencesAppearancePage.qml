// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle
import "../../config.js" as Config

ZrythmPreferencesPage {
  id: root

  property AppSettings appSettings: null
  title: qsTr("Appearance")

  readonly property var accentOptions: [
    { token: "zrythm-orange", name: qsTr("Zrythm Orange") },
    { token: "celestial-blue", name: qsTr("Celestial Blue") },
    { token: "jonquil-yellow", name: qsTr("Jonquil Yellow") },
    { token: "spring-green", name: qsTr("Spring Green") },
    { token: "munsell-red", name: qsTr("Munsell Red") },
    { token: "gunmetal", name: qsTr("Gunmetal") },
    { token: "electric-purple", name: qsTr("Electric Purple") }
  ]
  readonly property var availableAccentOptions: root.accentOptions.filter(
    (option) => ZrythmTheme.accentAvailableInMode(
      option.token, ZrythmTheme.darkMode))
  readonly property var languageEntries: {
    const makeEntry = (code) => {
      const info = Config.languageMap[code];
      return {
        "code": code,
        "displayName": code === "en" ? info.name : "\u200E" + info.name + "\u200E (" + info.percent + "%)"
      };
    };
    const mainEntries = Config.mainLanguageCodes.map(makeEntry);
    const moreCodes = Object.keys(Config.languageMap)
      .filter((code) => !Config.mainLanguageCodes.includes(code))
      .sort((a, b) => a.localeCompare(b));
    return [
      { "code": "", "displayName": qsTr("System") },
      ...mainEntries,
      ...moreCodes.map(makeEntry)
    ];
  }
  readonly property TranslationManager translationManager: GlobalState.application
    ? GlobalState.application.translationManager
    : null

  resources: [
    ButtonGroup {
      id: themeModeGroup
    }
  ]

  ActionRowGroup {
    title: qsTr("Theme")
    subtitle: qsTr("Base appearance and accent color")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Color scheme used across the interface")
      title: qsTr("Interface Theme")

      LinkedButtons {
        Button {
          ButtonGroup.group: themeModeGroup
          checkable: true
          checked: root.appSettings?.themeMode === AppSettings.ThemeMode.Dark
          text: qsTr("Dark")

          onToggled: {
            if (root.appSettings !== null)
              root.appSettings.themeMode = AppSettings.ThemeMode.Dark;
          }
        }

        Button {
          ButtonGroup.group: themeModeGroup
          checkable: true
          checked: root.appSettings?.themeMode === AppSettings.ThemeMode.Light
          text: qsTr("Light")

          onToggled: {
            if (root.appSettings !== null)
              root.appSettings.themeMode = AppSettings.ThemeMode.Light;
          }
        }
      }
    }

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Color used for selection, focus and active controls")
      title: qsTr("Accent Color")

      ComboBox {
        Layout.preferredWidth: 180
        currentIndex: root.accentIndex()
        model: root.availableAccentOptions
        textRole: "name"

        onActivated: (index) => {
          if (root.appSettings !== null)
            root.appSettings.accentToken = root.availableAccentOptions[index].token;
        }
      }
    }
  }

  ActionRowGroup {
    title: qsTr("Language & Text")
    subtitle: qsTr("Language used by the interface")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Language used for menus, labels and messages")
      title: qsTr("Language")

      ComboBox {
        Layout.preferredWidth: 220
        currentIndex: root.localeIndex()
        model: root.languageEntries
        textRole: "displayName"

        onActivated: (index) => {
          if (root.translationManager !== null)
            root.translationManager.loadTranslation(root.languageEntries[index].code);
        }
      }
    }
  }

  function accentIndex(): int {
    if (root.appSettings === null)
      return 0;
    const token = ZrythmTheme.effectiveAccentToken(
      root.appSettings.accentToken, ZrythmTheme.darkMode);
    return root.availableAccentOptions.findIndex(
      (option) => option.token === token);
  }

  function localeIndex(): int {
    const locale = root.appSettings?.uiLocale ?? "";
    const index = root.languageEntries.findIndex(
      (entry) => entry.code === locale);
    return index >= 0 ? index : 0;
  }
}
