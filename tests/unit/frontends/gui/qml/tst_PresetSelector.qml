// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import QmlTests

TestCase {
  id: test

  function test_arrows_emit_activated_without_applying() {
    const selector = createTemporaryObject(selectorComponent, test);
    activatedSpy.target = selector;
    activatedSpy.signalName = "activated";

    mouseClick(selector.nextButton);
    compare(activatedSpy.count, 1);
    compare(activatedSpy.signalArguments[0][0], 0);
    // Unidirectional flow: the control does not apply the selection
    compare(selector.currentIndex, -1);

    selector.currentIndex = 1;
    mouseClick(selector.prevButton);
    compare(activatedSpy.count, 2);
    compare(activatedSpy.signalArguments[1][0], 0);
    compare(selector.currentIndex, 1);
  }

  function test_arrows_wrap_around() {
    const selector = createTemporaryObject(selectorComponent, test);
    activatedSpy.target = selector;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();

    // At the first item, previous wraps to the last
    selector.currentIndex = 0;
    mouseClick(selector.prevButton);
    compare(activatedSpy.count, 1);
    compare(activatedSpy.signalArguments[0][0], 2);

    // At the last item, next wraps to the first
    selector.currentIndex = 2;
    mouseClick(selector.nextButton);
    compare(activatedSpy.count, 2);
    compare(activatedSpy.signalArguments[1][0], 0);
  }

  function test_empty_model_disables_control() {
    const selector = createTemporaryObject(selectorComponent, test);
    selector.model.clear();
    compare(selector.count, 0);
    verify(!selector.prevButton.enabled);
    verify(!selector.nextButton.enabled);
    verify(!selector.nameButton.enabled);
  }

  function test_external_popup_requests_instead_of_opening() {
    const selector = createTemporaryObject(selectorComponent, test);
    selector.externalPopup = true;
    requestSpy.target = selector;
    requestSpy.signalName = "popupRequested";

    mouseClick(selector.nameButton);
    compare(requestSpy.count, 1);
    // The in-scene popup stays closed; the host shows its own window
    compare(selector.popupVisible, false);
  }

  function test_initial_state() {
    const selector = createTemporaryObject(selectorComponent, test);
    verify(selector);
    compare(selector.count, 3);
    compare(selector.currentIndex, -1);
    // Both arrows are enabled and wrap around (from -1: next activates
    // the first item, previous the last)
    verify(selector.prevButton.enabled);
    verify(selector.nextButton.enabled);
    compare(selector.nameButton.text, selector.placeholderText);
  }

  function test_name_button_shows_current_text() {
    const selector = createTemporaryObject(defaultRoleComponent, test);
    verify(selector);
    compare(selector.count, 1);
    selector.currentIndex = 0;
    compare(selector.nameButton.text, "Only Preset");
  }

  function test_popup_keyboard_selection() {
    const selector = createTemporaryObject(selectorComponent, test);
    selector.currentIndex = 0;
    activatedSpy.target = selector;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();

    mouseClick(selector.nameButton);
    tryCompare(selector, "popupVisible", true);
    // Focus is set when the popup finishes opening (after the enter
    // transition), so wait for it before sending keys
    const browser = selector.popup.contentItem;
    tryVerify(() => browser.searchField.fieldActiveFocus);

    // Highlight follows currentIndex on open; Down moves to the next item
    keyClick(Qt.Key_Down);
    compare(browser.listView.currentIndex, 1);
    keyClick(Qt.Key_Return);
    compare(activatedSpy.count, 1);
    compare(activatedSpy.signalArguments[0][0], 1);
    tryCompare(selector, "popupVisible", false);
  }

  function test_popup_parent_alias() {
    const selector = createTemporaryObject(selectorComponent, test);
    const anchor = createTemporaryObject(anchorComponent, test);
    selector.popupParent = anchor;
    compare(selector.popup.parent, anchor);
    // Redirecting the parent alone does not change the popup type
    compare(selector.popup.popupType, Popup.Item);
  }

  function test_popup_passive_close_applies_selection() {
    const selector = createTemporaryObject(selectorComponent, test);
    selector.currentIndex = 0;
    activatedSpy.target = selector;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();

    mouseClick(selector.nameButton);
    tryCompare(selector, "popupVisible", true);
    const browser = selector.popup.contentItem;
    tryVerify(() => browser.searchField.fieldActiveFocus);

    // Navigating only selects (and would audition); the popup stays open
    keyClick(Qt.Key_Down);
    compare(browser.listView.currentIndex, 1);
    tryCompare(selector, "popupVisible", true);

    // Closing without an explicit apply/cancel commits the selection and
    // reports it through activated()
    selector.popup.close();
    tryCompare(selector, "popupVisible", false);
    tryCompare(activatedSpy, "count", 1);
    compare(activatedSpy.signalArguments[0][0], 1);
  }

  function test_tooltip_anchors_above_when_room() {
    const selector = createTemporaryObject(selectorComponent, test, {
      "y": 200
    });
    selector.currentIndex = 0;
    const tip = selector.nameToolTip;
    verify(tip);
    tip.delay = 0;

    mouseMove(selector.nameButton, selector.nameButton.width / 2, selector.nameButton.height / 2);
    tryVerify(() => tip.visible);
    compare(tip.y, -tip.implicitHeight - 3);
  }

  function test_tooltip_anchors_below_button_at_window_top() {
    const selector = createTemporaryObject(selectorComponent, test, {
      "y": 0
    });
    selector.currentIndex = 0;
    const tip = selector.nameToolTip;
    verify(tip);
    tip.delay = 0;

    mouseMove(selector.nameButton, selector.nameButton.width / 2, selector.nameButton.height / 2);
    tryVerify(() => tip.visible);
    // At the window top there is no room above, so it anchors below
    compare(tip.y, selector.nameButton.height + 3);
  }

  function test_tooltip_suppressed_in_external_popup_mode() {
    const selector = createTemporaryObject(selectorComponent, test);
    selector.currentIndex = 0;
    selector.externalPopup = true;
    const tip = selector.nameToolTip;
    verify(tip);
    tip.delay = 0;

    mouseMove(selector.nameButton, selector.nameButton.width / 2, selector.nameButton.height / 2);
    tryVerify(() => selector.nameButton.hovered);
    // Nothing fits inside a strip-height native scene: no tooltip
    verify(!tip.visible);
  }

  function test_window_popup_type() {
    const selector = createTemporaryObject(selectorComponent, test);
    // The default popup type stays in-scene
    compare(selector.popup.popupType, Popup.Item);
    selector.popupType = Popup.Window;
    compare(selector.popup.popupType, Popup.Window);

    selector.currentIndex = 0;
    activatedSpy.target = selector;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();

    mouseClick(selector.nameButton);
    tryCompare(selector, "popupVisible", true);
    const browser = selector.popup.contentItem;
    tryVerify(() => browser.searchField.fieldActiveFocus);

    keyClick(Qt.Key_Return);
    compare(activatedSpy.count, 1);
    compare(activatedSpy.signalArguments[0][0], 0);
    tryCompare(selector, "popupVisible", false);
  }

  height: 400
  name: "PresetSelector"
  // Mouse delivery requires the test object to be visible, and TestCase
  // items default to invisible
  visible: true
  when: windowShown
  width: 400

  SignalSpy {
    id: activatedSpy
  }

  SignalSpy {
    id: requestSpy
  }

  Component {
    id: selectorComponent

    PresetSelector {
      currentText: currentIndex >= 0 ? presetModel.get(currentIndex).name : ""

      model: ListModel {
        id: presetModel

        ListElement {
          name: "Preset A"
        }

        ListElement {
          name: "Preset B"
        }

        ListElement {
          name: "Preset C"
        }
      }
    }
  }

  Component {
    id: defaultRoleComponent

    PresetSelector {
      currentText: currentIndex >= 0 ? defaultRoleModel.get(currentIndex).display : ""

      model: ListModel {
        id: defaultRoleModel

        ListElement {
          display: "Only Preset"
        }
      }
    }
  }

  Component {
    id: anchorComponent

    Item {
    }
  }
}
