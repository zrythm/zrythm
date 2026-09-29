// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest

TestCase {
  id: test

  name: "Menu"
  height: 300
  visible: true
  when: windowShown
  width: 400

  Component {
    id: menuComponent

    Menu {
      MenuItem {
        text: qsTr("First")
      }

      MenuSeparator {
      }

      MenuItem {
        text: qsTr("Second")
      }
    }
  }

  function createOpenMenu() {
    const menu = createTemporaryObject(menuComponent, test);
    verify(menu);
    menu.open();
    tryVerify(() => menu.visible);
    verify(waitForItemPolished(menu.contentItem));
    return menu;
  }

  function findSeparator(menu) {
    for (let i = 0; i < menu.count; ++i) {
      const item = menu.itemAt(i);
      if (item instanceof MenuSeparator)
        return item;
    }
    return null;
  }

  function test_items_are_padded_from_the_popup_edges() {
    const menu = createOpenMenu();
    compare(menu.padding, 4);
    compare(menu.contentItem.x, 4);
    compare(menu.contentItem.y, 4);
    compare(menu.availableWidth, menu.width - 8);
  }

  function test_separator_spans_between_item_margins() {
    const menu = createOpenMenu();
    const separator = findSeparator(menu);
    verify(separator);
    compare(separator.height, 9);
    const line = separator.contentItem;
    compare(line.x, 1);
    compare(line.width, separator.width - 2);
  }
}
