// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import Zrythm

TestCase {
  id: test

  name: "PreferencesAdvancedPage"
  height: 500
  visible: true
  when: windowShown
  width: 600

  Component {
    id: pageComponent

    PreferencesAdvancedPage {
      appSettings: testAppSettings
    }
  }

  function init(): void {
    testAppSettings.reset_preference_settings();
    testAppSettings.transportReturnToCue = false;
    testAppSettings.showCacheActivity = true;
  }

  function test_reset_requires_confirmation_and_cancel_preserves_values() {
    const page = createTemporaryObject(pageComponent, test);
    verify(page);
    const resetButton = findChild(page, "resetPreferencesButton");
    const dialog = findChild(page, "resetPreferencesDialog");
    const cancelButton = findChild(page, "resetPreferencesCancelButton");
    verify(resetButton);
    verify(dialog);
    verify(cancelButton);

    resetButton.clicked();
    tryVerify(() => dialog.visible);
    verify(!testAppSettings.transportReturnToCue);
    verify(testAppSettings.showCacheActivity);

    cancelButton.clicked();
    tryVerify(() => !dialog.visible);
    verify(!testAppSettings.transportReturnToCue);
    verify(testAppSettings.showCacheActivity);
  }

  function test_confirmed_reset_restores_values() {
    const page = createTemporaryObject(pageComponent, test);
    verify(page);
    const resetButton = findChild(page, "resetPreferencesButton");
    const dialog = findChild(page, "resetPreferencesDialog");
    const confirmButton = findChild(page, "resetPreferencesConfirmButton");
    verify(resetButton);
    verify(dialog);
    verify(confirmButton);

    resetButton.clicked();
    tryVerify(() => dialog.visible);
    confirmButton.clicked();
    tryVerify(() => !dialog.visible);
    verify(testAppSettings.transportReturnToCue);
    verify(!testAppSettings.showCacheActivity);
  }
}
