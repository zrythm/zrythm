// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import Zrythm
import ZrythmStyle

TestCase {
  id: test

  name: "EditableSlider"

  Component {
    id: windowComponent

    ApplicationWindow {
      property alias control: control
      property alias editedSpy: editedSpy

      EditableSlider {
        id: control

        decimals: 2
        from: -100
        stepSize: 1
        to: 100
        value: 10

        SignalSpy {
          id: editedSpy

          signalName: "valueEdited"
          target: control
        }
      }

      height: 200
      visible: true
      width: 400
    }
  }

  property ApplicationWindow window: null

  function init() {
    window = windowComponent.createObject(null);
    tryVerify(() => window.visible);
  }

  function cleanup() {
    window.destroy();
    window = null;
  }

  function readoutLabel() {
    return findChild(window.control, "readoutLabel");
  }

  function editDialog() {
    return findChild(window.control, "editDialog");
  }

  function valueField(dialog) {
    return findChild(dialog, "valueField");
  }

  function typeDigits(text) {
    for (let i = 0; i < text.length; i++)
      keyClick(text.charCodeAt(i));
  }

  function test_readout_shows_initial_value() {
    compare(readoutLabel().text, "10.00");
  }

  function test_readout_follows_value_changes() {
    window.control.value = 42.5;
    tryCompare(readoutLabel(), "text", "42.50");
  }

  function test_drag_emits_value_edited() {
    const width = window.control.width;
    mouseDrag(window.control, width * 0.25, window.control.height / 2, width * 0.5, 0, Qt.LeftButton);
    verify(window.editedSpy.count >= 1);
    verify(window.control.value !== 10);
    compare(readoutLabel().text, window.control._formatted);
  }

  function test_double_click_readout_applies_typed_value() {
    const label = readoutLabel();
    mouseDoubleClickSequence(label, label.width / 2, label.height / 2, Qt.LeftButton);
    const dialog = editDialog();
    tryCompare(dialog, "visible", true);
    tryVerify(() => valueField(dialog).activeFocus);
    // this harness drops the first key event sent after a dialog opened
    // on a double-click sequence, so send a harmless modifier-only
    // keystroke first
    keyClick(Qt.Key_Shift);
    valueField(dialog).text = "73";
    keyClick(Qt.Key_Enter);
    tryCompare(window.editedSpy, "count", 1);
    compare(window.editedSpy.signalArguments[0][0], 73);
    tryCompare(readoutLabel(), "text", "73.00");
  }

  function test_dialog_rejects_value_out_of_range() {
    const label = readoutLabel();
    mouseDoubleClickSequence(label, label.width / 2, label.height / 2, Qt.LeftButton);
    const dialog = editDialog();
    tryCompare(dialog, "visible", true);
    tryVerify(() => valueField(dialog).activeFocus);
    typeDigits("999");
    // the OK button is disabled while the input is out of range, so
    // accepting does nothing
    keyClick(Qt.Key_Enter);
    tryCompare(dialog, "visible", true);
    compare(window.editedSpy.count, 0);
    keyClick(Qt.Key_Escape);
    tryCompare(dialog, "visible", false);
    compare(window.editedSpy.count, 0);
    compare(readoutLabel().text, "10.00");
  }
}
