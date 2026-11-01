// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import Zrythm

TestCase {
  id: test

  name: "PreferencesDialog"
  height: 640
  visible: true
  when: windowShown
  width: 880

  Component {
    id: dialogComponent

    PreferencesDialog {
    }
  }

  function makeDialog(): PreferencesDialog {
    const dialog = createTemporaryObject(dialogComponent, test, {
      "appSettings": testAppSettings
    });
    verify(dialog);
    return dialog;
  }

  function test_search_index_comes_from_setting_rows() {
    const dialog = makeDialog();
    const entries = dialog.collectSearchEntries("cue");
    compare(entries.length, 1);
    compare(entries[0].categoryTitle, "Playback & Mixing");
    compare(entries[0].group, "Transport");
    compare(entries[0].title, "Return To Cue");
  }

  function test_search_matches_row_terms() {
    const dialog = makeDialog();
    const entries = dialog.collectSearchEntries("lv2");
    compare(entries.length, 1);
    compare(entries[0].categoryTitle, "Plugins");
    compare(entries[0].title, "LV2 Paths");
  }

  function test_visible_results_follow_the_search_field() {
    const dialog = makeDialog();
    const field = findChild(dialog, "preferencesSearchField");
    verify(field);
    field.text = "undo";
    tryVerify(() => dialog.visibleSearchEntries.length === 1);
    compare(dialog.visibleSearchEntries[0].title, "Undo Stack Length");
  }

  function test_reset_search_closes_the_dialog_only_when_search_is_empty() {
    const dialog = makeDialog();
    const field = findChild(dialog, "preferencesSearchField");
    verify(field);
    dialog.open();
    tryVerify(() => dialog.visible);
    field.text = "cue";
    dialog.resetSearchOrClose();
    compare(field.text, "");
    verify(dialog.visible);
    dialog.resetSearchOrClose();
    tryVerify(() => !dialog.visible);
  }
}
