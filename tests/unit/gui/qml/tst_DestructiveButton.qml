// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import Zrythm
import ZrythmStyle

TestCase {
  id: test

  name: "DestructiveButton"

  Component {
    id: windowComponent

    ApplicationWindow {
      property alias control: control

      DestructiveButton {
        id: control

        // hover off so the offscreen cursor at (0, 0) cannot blend the
        // fill under test
        hoverEnabled: false
        text: qsTr("Delete")
      }

      height: 60
      visible: true
      width: 200
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

  function test_resting_uses_base_fill_with_error_text() {
    const control = window.control;
    compare(control.background.color, control.palette.button);
    compare(control.contentItem.color, ZrythmTheme.errorTextColor);
    verify(ZrythmTheme.contrastRatio(control.contentItem.color, control.background.color) >= 4.5);
  }

  function test_checked_uses_error_fill_with_polarity_text() {
    const control = window.control;
    control.checkable = true;
    control.checked = true;
    tryCompare(control.background, "color", ZrythmTheme.errorColor);
    tryCompare(control.contentItem, "color", ZrythmTheme.polarityTextColor(ZrythmTheme.errorColor));
    verify(ZrythmTheme.contrastRatio(control.contentItem.color, control.background.color) >= 4.5);
  }
}
