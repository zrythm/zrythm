// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import ZrythmStyle 1.0

TestCase {
  id: test

  function findShortcutLabel(item) {
    const stack = [item];
    while (stack.length > 0) {
      const current = stack.pop();
      if (current instanceof Text)
        return current;
      for (let i = 0; i < current.children.length; ++i)
        stack.push(current.children[i]);
    }
    return null;
  }

  function createShownItem(component) {
    const win = createTemporaryObject(component, test);
    verify(win);
    win.show();
    tryVerify(() => win.visible);
    verify(win.menuItem);
    return win;
  }

  function test_shortcut_label_shows_action_shortcut() {
    const win = createShownItem(itemWithActionComponent);
    const label = findShortcutLabel(win.menuItem.contentItem);
    verify(label);
    verify(label.visible);
    compare(label.text, "Ctrl+Z");
  }

  function test_shortcut_label_uses_faded_typography() {
    const win = createShownItem(itemWithActionComponent);
    const label = findShortcutLabel(win.menuItem.contentItem);
    verify(label);
    compare(label.font.pixelSize, ZrythmTheme.fadedTextFont.pixelSize);
    compare(label.font.weight, ZrythmTheme.fadedTextFont.weight);
  }

  function test_no_visible_shortcut_label_without_action() {
    const win = createShownItem(plainItemComponent);
    const label = findShortcutLabel(win.menuItem.contentItem);
    verify(!label || !label.visible);
  }

  function test_enum_shortcut_resolves_to_platform_text() {
    const win = createShownItem(enumShortcutItemComponent);
    const label = findShortcutLabel(win.menuItem.contentItem);
    verify(label);
    verify(label.visible);
    verify(label.text !== "");
    verify(label.text !== "5");
  }

  Component {
    id: enumShortcutItemComponent

    ApplicationWindow {
      width: 300
      height: 100
      visible: true

      property alias menuItem: menuItem

      MenuItem {
        id: menuItem

        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Save")

        action: Action {
          shortcut: StandardKey.Save
          text: qsTr("Save")
        }
      }
    }
  }

  Component {
    id: itemWithActionComponent

    ApplicationWindow {
      width: 300
      height: 100
      visible: true

      property alias menuItem: menuItem

      MenuItem {
        id: menuItem

        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Undo")

        action: Action {
          shortcut: "Ctrl+Z"
          text: qsTr("Undo")
        }
      }
    }
  }

  Component {
    id: plainItemComponent

    ApplicationWindow {
      width: 300
      height: 100
      visible: true

      property alias menuItem: menuItem

      MenuItem {
        id: menuItem

        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Plain")
      }
    }
  }
}
