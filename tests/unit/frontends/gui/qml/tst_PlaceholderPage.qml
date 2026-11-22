// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import QtTest
import QmlTests

TestCase {
  id: test

  name: "PlaceholderPage"
  height: 300
  visible: true
  when: windowShown
  width: 400

  function test_icon_tinted_by_default() {
    const page = createTemporaryObject(pageComponent, test);
    verify(page);
    tryCompare(page.icon, "color", page.palette.buttonText);
  }

  function test_icon_not_tinted_when_tint_disabled() {
    const page = createTemporaryObject(pageComponent, test, {
      "tintIcon": false
    });
    verify(page);
    tryCompare(page.icon, "color", Qt.rgba(0, 0, 0, 0));
  }

  Component {
    id: pageComponent

    PlaceholderPage {
      buttonText: "Start"
      description: "A description that is long enough to wrap"
      title: "Title"
    }
  }

  function contentColumn(page) {
    const column = page.children[0];
    verify(column instanceof ColumnLayout);
    return column;
  }

  function test_fills_parent() {
    const page = createTemporaryObject(pageComponent, test);
    verify(page);
    tryCompare(page, "width", test.width);
    tryCompare(page, "height", test.height);
  }

  function test_column_width_capped_to_fraction_of_page() {
    const page = createTemporaryObject(pageComponent, test);
    verify(page);
    const column = contentColumn(page);
    // min(width - 40, width * 0.8) == min(360, 320) == 320 for width 400
    tryCompare(column, "width", 320);
  }

  function test_column_centered() {
    const page = createTemporaryObject(pageComponent, test);
    verify(page);
    const column = contentColumn(page);
    tryVerify(() => column.height > 0);
    // anchors.centerIn aligns to whole pixels by default (alignWhenCentered),
    // so allow up to half a pixel of deviation from the ideal center
    tryVerify(() => Math.abs(column.x - (test.width - column.width) / 2) <= 0.5);
    tryVerify(() => Math.abs(column.y - (test.height - column.height) / 2) <= 0.5);
  }
}
