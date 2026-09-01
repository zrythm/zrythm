// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import QmlTests

TestCase {
  id: test

  function test_click_activates() {
    const popup = createTemporaryObject(popupComponent, test);
    activatedSpy.target = popup;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();

    tryVerify(() => popup.listView.activeFocus);
    const delegate = popup.listView.itemAtIndex(1);
    verify(delegate);
    mouseClick(delegate);
    compare(activatedSpy.count, 1);
    compare(activatedSpy.signalArguments[0][0], 1);
  }

  function test_escape_requests_close() {
    const popup = createTemporaryObject(popupComponent, test);
    closeSpy.target = popup;
    closeSpy.signalName = "closeRequested";
    closeSpy.clear();

    tryVerify(() => popup.listView.activeFocus);
    keyClick(Qt.Key_Escape);
    compare(closeSpy.count, 1);
  }

  function test_group_headers_hidden_by_default() {
    const popup = createTemporaryObject(ungroupedPopupComponent, test);
    tryVerify(() => popup.listView.itemAtIndex(3));
    compare(visibleGroupHeaders(popup.listView).length, 0);
  }

  function test_group_headers_shown_for_named_groups() {
    const popup = createTemporaryObject(groupedPopupComponent, test);
    tryVerify(() => popup.listView.itemAtIndex(3));

    const headers = visibleGroupHeaders(popup.listView);
    compare(headers.length, 2);
    compare(headers[0].section, "Factory");
    compare(headers[1].section, "User");

    // Each header sits directly above its group's first item
    tryVerify(() => Math.abs(headers[0].y + headers[0].height - popup.listView.itemAtIndex(1).y) < 1);
    tryVerify(() => Math.abs(headers[1].y + headers[1].height - popup.listView.itemAtIndex(3).y) < 1);
  }

  function test_highlight_follows_current_index() {
    const popup = createTemporaryObject(popupComponent, test);
    popup.currentIndex = 2;
    compare(popup.listView.currentIndex, 2);
    const delegate = popup.listView.itemAtIndex(2);
    tryVerify(() => delegate && delegate.highlighted);
  }

  function visibleGroupHeaders(listView) {
    const result = [];
    for (const child of listView.contentItem.children) {
      if (child.objectName === "groupHeader" && child.visible)
        result.push(child);
    }
    return result;
  }

  height: 400
  name: "PresetListPopup"
  // Mouse delivery and active focus require the test object to be visible,
  // and TestCase items default to invisible
  visible: true
  when: windowShown
  width: 400

  SignalSpy {
    id: activatedSpy
  }

  SignalSpy {
    id: closeSpy
  }

  Component {
    id: groupedPopupComponent

    PresetListPopup {
      height: implicitHeight
      showGroupHeaders: true
      textRole: "name"
      width: implicitWidth

      model: ListModel {
        ListElement {
          group: ""
          name: "Solo"
        }

        ListElement {
          group: "Factory"
          name: "Init"
        }

        ListElement {
          group: "Factory"
          name: "Bright"
        }

        ListElement {
          group: "User"
          name: "Lead"
        }
      }
    }
  }

  Component {
    id: ungroupedPopupComponent

    PresetListPopup {
      height: implicitHeight
      textRole: "name"
      width: implicitWidth

      model: ListModel {
        ListElement {
          group: ""
          name: "Solo"
        }

        ListElement {
          group: "Factory"
          name: "Init"
        }

        ListElement {
          group: "Factory"
          name: "Bright"
        }

        ListElement {
          group: "User"
          name: "Lead"
        }
      }
    }
  }

  Component {
    id: popupComponent

    PresetListPopup {
      height: implicitHeight
      textRole: "name"
      width: implicitWidth

      model: ListModel {
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
}
