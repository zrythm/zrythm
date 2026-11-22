// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmStyle

Item {
  id: root

  required property AppSettings appSettings

  Component.onCompleted: root.applyTheme()

  Connections {
    function onAccentTokenChanged(): void {
      root.applyTheme();
    }

    function onThemeModeChanged(): void {
      root.applyTheme();
    }

    target: root.appSettings
  }

  function applyTheme(): void {
    ZrythmTheme.darkMode =
      root.appSettings.themeMode === AppSettings.ThemeMode.Dark;
    ZrythmTheme.primaryColor = ZrythmTheme.accentColorForToken(
      ZrythmTheme.effectiveAccentToken(
        root.appSettings.accentToken, ZrythmTheme.darkMode));
  }
}
