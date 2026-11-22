// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import QmlTests
import ZrythmStyle

TestCase {
  id: test

  name: "StatusBar"
  height: 100
  visible: true
  when: windowShown
  width: 400

  Component {
    id: barComponent

    StatusBar {
      width: 400

      leftItems: [
        StatusBarText {
          text: "left one"
        },
        StatusBarText {
          text: "left two"
        }
      ]
      rightItems: [
        StatusBarText {
          text: "right"
        }
      ]
    }
  }

  Component {
    id: statusTextComponent

    StatusBarText {
      text: "text"
    }
  }

  function test_sections_place_items_at_opposite_ends() {
    const bar = createTemporaryObject(barComponent, test);
    verify(bar);
    verify(waitForItemPolished(bar));
    const firstLeft = bar.leftItems[0];
    const secondLeft = bar.leftItems[1];
    const right = bar.rightItems[0];
    verify(firstLeft);
    verify(secondLeft);
    verify(right);
    const firstLeftPos = firstLeft.mapToItem(bar, 0, 0);
    const secondLeftPos = secondLeft.mapToItem(bar, 0, 0);
    const rightPos = right.mapToItem(bar, 0, 0);
    // The left section starts at the leading edge and keeps declaration order
    verify(firstLeftPos.x < bar.width / 4);
    verify(secondLeftPos.x > firstLeftPos.x);
    // The right section ends at the trailing edge
    verify(rightPos.x > bar.width / 2);
    verify(rightPos.x + right.width > bar.width * 0.75);
    verify(rightPos.x > secondLeftPos.x);
  }

  function test_text_uses_faded_small_font() {
    const item = createTemporaryObject(statusTextComponent, test);
    verify(item);
    compare(item.font.pixelSize, 9);
    // Compare string forms: alpha-bearing colors quantize differently
    // depending on their value source
    verify(item.color.toString() === ZrythmTheme.placeholderTextColor.toString());
  }
}
