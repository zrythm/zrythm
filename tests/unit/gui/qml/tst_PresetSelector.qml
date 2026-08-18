// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import QmlTests

TestCase {
  id: test

  function test_arrows_clamp_at_bounds() {
    const selector = createTemporaryObject(selectorComponent, test);
    selector.currentIndex = 0;
    verify(!selector.prevButton.enabled);
    verify(selector.nextButton.enabled);
    selector.currentIndex = 2;
    verify(selector.prevButton.enabled);
    verify(!selector.nextButton.enabled);
  }

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

  function test_default_text_role() {
    const selector = createTemporaryObject(defaultRoleComponent, test);
    verify(selector);
    compare(selector.count, 1);
    selector.currentIndex = 0;
    compare(selector.nameButton.text, "Only Preset");
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
    verify(!selector.prevButton.enabled);
    // Next from -1 activates the first item
    verify(selector.nextButton.enabled);
    compare(selector.nameButton.text, selector.placeholderText);
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
    const listView = selector.popup.contentItem.listView;
    tryVerify(() => listView.activeFocus);

    // Highlight follows currentIndex on open; Down moves to the next item
    keyClick(Qt.Key_Down);
    compare(listView.currentIndex, 1);
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
    const listView = selector.popup.contentItem.listView;
    tryVerify(() => listView.activeFocus);

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
      textRole: "name"

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
