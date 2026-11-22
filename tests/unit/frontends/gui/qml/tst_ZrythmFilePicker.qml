// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import QtTest
import Zrythm

TestCase {
  id: test

  name: "ZrythmFilePicker"

  Component {
    id: pickerComponent

    ZrythmFilePicker {
      initialPath: "/tmp/zrythm-file-picker-test"
    }
  }

  Component {
    id: emptyPickerComponent

    ZrythmFilePicker {
      initialPath: ""
    }
  }

  Component {
    id: pickerWithDefaultComponent

    ZrythmFilePicker {
      defaultPath: "/tmp/zrythm-file-picker-default"
      initialPath: "/tmp/zrythm-file-picker-test"
    }
  }

  function findTextField(item: Item): T.TextField {
    for (let i = 0; i < item.data.length; ++i) {
      const child = item.data[i];
      if (child instanceof T.TextField)
        return child;
    }
    return null;
  }

  function findResetButton(item: Item): T.Button {
    for (let i = 0; i < item.data.length; ++i) {
      const child = item.data[i];
      if (child instanceof Loader && child.item instanceof T.Button && child.item.icon.source.toString().includes("revert-symbolic.svg"))
        return child.item;
    }
    return null;
  }

  function test_selected_url_starts_at_initial_path() {
    const picker = createTemporaryObject(pickerComponent, test);
    verify(picker);
    compare(picker.selectedUrl, QmlUtils.localFileToQUrl("/tmp/zrythm-file-picker-test"));
  }

  function test_text_field_shows_initial_path() {
    const picker = createTemporaryObject(pickerComponent, test);
    verify(picker);
    const field = findTextField(picker);
    verify(field);
    compare(field.text, "/tmp/zrythm-file-picker-test");
  }

  function test_text_field_shows_placeholder_without_initial_path() {
    const picker = createTemporaryObject(emptyPickerComponent, test);
    verify(picker);
    const field = findTextField(picker);
    verify(field);
    compare(field.text, "(no path selected)");
  }

  function test_reset_button_absent_without_default_path() {
    const picker = createTemporaryObject(pickerComponent, test);
    verify(picker);
    compare(findResetButton(picker), null);
  }

  function test_reset_button_present_with_default_path() {
    const picker = createTemporaryObject(pickerWithDefaultComponent, test);
    verify(picker);
    tryVerify(() => findResetButton(picker) !== null);
  }

  function test_reset_button_restores_default_path() {
    const picker = createTemporaryObject(pickerWithDefaultComponent, test);
    verify(picker);
    picker.selectedUrl = QmlUtils.localFileToQUrl("/tmp/somewhere-else");
    const button = findResetButton(picker);
    tryVerify(() => findResetButton(picker) !== null);
    button.clicked();
    compare(picker.selectedUrl, QmlUtils.localFileToQUrl("/tmp/zrythm-file-picker-default"));
  }
}
