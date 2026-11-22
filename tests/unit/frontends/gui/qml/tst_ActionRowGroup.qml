// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtTest
import QmlTests

TestCase {
  id: test

  name: "ActionRowGroup"
  height: 300
  visible: true
  when: windowShown
  width: 400

  function collectAll(item, klass) {
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

  function findCard(group) {
    const rects = collectAll(group, Rectangle);
    return rects.find((r) => r.radius > 0) ?? null;
  }

  function test_title_and_subtitle_shown() {
    const group = createTemporaryObject(groupComponent, test, {
      "title": "Recent Projects",
      "subtitle": "Projects you opened recently"
    });
    verify(group);
    const texts = collectAll(group, Text);
    verify(texts.some((t) => t.text === "Recent Projects" && t.visible));
    verify(texts.some((t) => t.text === "Projects you opened recently" && t.visible));
  }

  function test_subtitle_hidden_when_empty() {
    const group = createTemporaryObject(groupComponent, test, {
      "title": "Recent Projects"
    });
    verify(group);
    const texts = collectAll(group, Text);
    verify(texts.some((t) => t.text === "Recent Projects" && t.visible));
    verify(!texts.some((t) => t.text === "" && t.visible));
  }

  function test_rows_inside_card() {
    const group = createTemporaryObject(groupComponent, test);
    verify(group);
    const card = findCard(group);
    verify(card);
    const rows = collectAll(group, T.ItemDelegate);
    compare(rows.length, 2);
    for (const row of rows) {
      let parentItem = row.parent;
      let insideCard = false;
      while (parentItem) {
        if (parentItem === card) {
          insideCard = true;
          break;
        }
        parentItem = parentItem.parent;
      }
      verify(insideCard);
    }
  }

  function test_suffix_action_button_visible_at_rest() {
    const wrapper = createTemporaryObject(suffixGroupComponent, test);
    verify(wrapper);
    const buttons = collectAll(wrapper.group, T.ToolButton);
    const clearButton = buttons.find((b) => b.action?.text === "Clear");
    verify(clearButton);
    // Group-level actions stay visible without hover
    verify(clearButton.visible);
  }

  Component {
    id: groupComponent

    ActionRowGroup {
      width: 380

      ActionRow {
        title: "Row A"
      }

      ActionRow {
        interactive: true
        title: "Row B"
      }
    }
  }

  Component {
    id: suffixGroupComponent

    Item {
      property alias group: group

      width: 380

      ActionRowGroup {
        id: group

        width: parent.width
        suffixActions: [clearAction]
        title: "Recent Projects"

        ActionRow {
          title: "Row A"
        }
      }

      Action {
        id: clearAction

        text: "Clear"
      }
    }
  }
}
