// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtTest
import QmlTests

TestCase {
  id: test

  name: "ActionRow"
  height: 300
  visible: true
  when: windowShown
  width: 400

  function collectAll (item, klass) {
    const result = [];
    const stack = [item];
    while (stack.length > 0) {
      const current = stack.pop();
      if (current instanceof klass)
        result.push(current);
      for (let i = 0; i < current.children.length; ++i)
        stack.push(current.children[i]);
    }
    return result;
  }

  function findMenu (obj) {
    for (let i = 0; i < obj.data.length; ++i) {
      if (obj.data[i] instanceof T.Menu)
        return obj.data[i];
    }
    return null;
  }

  function test_form_mode_sizes_to_content () {
    const row = createTemporaryObject(formRowComponent, test);
    verify(row);
    verify(row.implicitHeight > 0);
    verify(row.implicitHeight < 40);
  }

  function test_form_mode_suffix_controls_visible () {
    const row = createTemporaryObject(formRowComponent, test);
    verify(row);
    const fields = collectAll(row, T.TextField);
    compare(fields.length, 1);
    verify(fields[0].visible);
  }

  function test_subtitle_fallback_used_when_subtitle_empty () {
    const row = createTemporaryObject(formRowComponent, test, {
      "title": "Graphic EQ",
      "subtitle": "",
      "subtitleFallback": "LV2"
    });
    verify(row);
    const texts = collectAll(row, Text);
    const subtitle = texts.find((t) => t.text === "LV2");
    verify(subtitle);
    verify(subtitle.visible);
  }

  function test_subtitle_shown_when_set () {
    const row = createTemporaryObject(formRowComponent, test, {
      "subtitle": "Zrythm",
      "subtitleFallback": "LV2"
    });
    verify(row);
    const texts = collectAll(row, Text);
    verify(texts.some((t) => t.text === "Zrythm"));
    verify(!texts.some((t) => t.text === "LV2"));
  }

  function test_list_mode_uses_fixed_row_height () {
    const row = createTemporaryObject(listRowComponent, test);
    verify(row);
    compare(row.row.implicitHeight, 40);
  }

  function test_checked_suffix_action_visible_at_rest () {
    const wrapper = createTemporaryObject(listRowComponent, test);
    verify(wrapper);
    const buttons = collectAll(wrapper.row, T.ToolButton);
    // star + insert + "…" overflow
    compare(buttons.length, 3);
    const starButton = buttons.find((b) => b.action?.text === "Favorite");
    const insertButton = buttons.find((b) => b.action?.text === "Insert");
    const overflowButton = buttons.find((b) => b.action === null);
    verify(starButton);
    verify(insertButton);
    verify(overflowButton);
    // Checked (favorited) stays visible without hover; the rest stay hidden
    verify(starButton.visible);
    verify(!insertButton.visible);
    verify(!overflowButton.visible);
  }

  function test_overflow_menu_contains_overflow_actions () {
    const wrapper = createTemporaryObject(listRowComponent, test);
    verify(wrapper);
    const menu = findMenu(wrapper.row);
    verify(menu);
    compare(menu.count, 2);
  }

  Component {
    id: formRowComponent

    ActionRow {
      title: "Title"
      subtitle: "Track title"

      TextField {
        placeholderText: qsTr("Enter title...")
      }
    }
  }

  Component {
    id: listRowComponent

    Item {
      property alias row: row

      width: 380

      ActionRow {
        id: row

        width: parent.width
        interactive: true
        title: "Analog Synth"
        subtitle: "Zrythm"
        suffixActions: [starAction, insertAction]
        overflowActions: [infoAction, revealAction]
      }

      Action {
        id: starAction

        checkable: true
        checked: true
        text: "Favorite"
      }

      Action {
        id: insertAction

        text: "Insert"
      }

      Action {
        id: infoAction

        text: "Plugin Info"
      }

      Action {
        id: revealAction

        text: "Show in File Manager"
      }
    }
  }
}
