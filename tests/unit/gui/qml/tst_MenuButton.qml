// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import Zrythm
import ZrythmStyle

TestCase {
  id: test

  name: "MenuButton"

  Component {
    id: windowComponent

    ApplicationWindow {
      property alias button: button
      property alias menu: testMenu
      property alias popup: testPopup

      height: 100
      visible: true
      width: 300

      MenuButton {
        id: button

        anchors.centerIn: parent
        text: "Filters"

        Menu {
          id: testMenu

          MenuItem {
            text: "Item"
          }
        }

        Popup {
          id: testPopup

          Label {
            text: "Item"
          }
        }
      }
    }
  }

  property ApplicationWindow window: null

  function init() {
    window = windowComponent.createObject(null);
    tryVerify(() => window.visible);
  }

  function cleanup() {
    window.menu.close();
    window.popup.close();
    window.destroy();
    window = null;
  }

  function test_trailing_chevron_present() {
    const chevron = window.button.contentItem.children[1];
    verify(String(chevron.source).indexOf("chevron-down") !== -1);
    compare(chevron.width, 12);
  }

  function test_click_opens_menu() {
    window.button.menu = window.menu;
    verify(!window.menu.opened);
    mouseClick(window.button);
    tryVerify(() => window.menu.opened);
  }

  function test_click_opens_plain_popup() {
    window.button.menu = window.popup;
    verify(!window.popup.visible);
    mouseClick(window.button);
    tryVerify(() => window.popup.visible);
  }

  function test_click_without_menu_keeps_popups_closed() {
    window.button.menu = null;
    mouseClick(window.button);
    verify(!window.menu.opened);
    verify(!window.popup.visible);
  }

  function test_active_tints_label_and_chevron_accent() {
    const label = window.button.contentItem.children[0];
    const chevron = window.button.contentItem.children[1];
    window.button.menu = null;
    window.button.active = true;
    tryCompare(label, "color", ZrythmTheme.colorPalette.accent);
    tryCompare(chevron, "color", ZrythmTheme.colorPalette.accent);
  }
}
