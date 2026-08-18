// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
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

  function test_highlight_follows_current_index() {
    const popup = createTemporaryObject(popupComponent, test);
    popup.currentIndex = 2;
    compare(popup.listView.currentIndex, 2);
    const delegate = popup.listView.itemAtIndex(2);
    tryVerify(() => delegate && delegate.highlighted);
  }

  function test_popup_host_invokables() {
    const host = createTemporaryObject(hostComponent, test);
    const popup = createTemporaryObject(popupComponent, test, {
      popupHost: host
    });
    hostActivatedSpy.target = host;
    hostActivatedSpy.signalName = "activatedReceived";
    hostActivatedSpy.clear();
    hostDismissedSpy.target = host;
    hostDismissedSpy.signalName = "dismissedReceived";
    hostDismissedSpy.clear();

    tryVerify(() => popup.listView.activeFocus);

    // Clicking a delegate calls the host's presetPopupActivated()
    const delegate = popup.listView.itemAtIndex(1);
    mouseClick(delegate);
    compare(hostActivatedSpy.count, 1);
    compare(hostActivatedSpy.signalArguments[0][0], 1);

    // Escape calls the host's presetPopupDismissed()
    keyClick(Qt.Key_Escape);
    compare(hostDismissedSpy.count, 1);
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

  SignalSpy {
    id: hostActivatedSpy
  }

  SignalSpy {
    id: hostDismissedSpy
  }

  // Stand-in for the C++ PresetPopupController passed to windowed popups
  Component {
    id: hostComponent

    QtObject {
      signal activatedReceived(int index)
      signal dismissedReceived

      function presetPopupActivated(index) {
        activatedReceived(index);
      }
      function presetPopupDismissed() {
        dismissedReceived();
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
