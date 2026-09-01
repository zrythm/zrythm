// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import QmlTests

TestCase {
  id: test

  function findItem(item, name) {
    if (item.objectName === name)
      return item;
    for (let i = 0; i < item.children.length; i++) {
      const found = findItem(item.children[i], name);
      if (found)
        return found;
    }
    return null;
  }

  function makeBrowser(extraProps) {
    const host = createTemporaryObject(hostComponent, test);
    const props = extraProps || {};
    props.popupHost = host;
    const browser = createTemporaryObject(browserComponent, test, props);
    return {
      "browser": browser,
      "host": host
    };
  }

  function test_cancel_button_reverts() {
    fakePlugin.reset();
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    closeSpy.target = ctx.browser;
    closeSpy.signalName = "closeRequested";
    closeSpy.clear();
    hostDismissedSpy.target = ctx.host;
    hostDismissedSpy.signalName = "dismissedReceived";
    hostDismissedSpy.clear();

    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    keyClick(Qt.Key_Down);
    tryVerify(() => fakePlugin.presetIndex === 1);

    const cancelButton = findItem(ctx.browser, "cancelButton");
    verify(cancelButton);
    mouseClick(cancelButton);
    compare(fakePlugin.revertCount, 1);
    compare(fakePlugin.commitCount, 0);
    compare(closeSpy.count, 1);
    compare(hostDismissedSpy.count, 1);
  }

  function test_click_selects_and_auditions_without_closing() {
    fakePlugin.reset();
    const ctx = makeBrowser();
    hostActivatedSpy.target = ctx.host;
    hostActivatedSpy.signalName = "activatedReceived";
    hostActivatedSpy.clear();

    // With a filter active, clicking a row auditions its source-model row
    // and keeps the browser open
    ctx.browser.searchField.text = "fac";
    tryCompare(ctx.browser, "count", 2);
    const delegate = ctx.browser.listView.itemAtIndex(1);
    verify(delegate);
    mouseClick(delegate);
    tryVerify(() => fakePlugin.presetIndex === 2);
    compare(hostActivatedSpy.count, 0);
    compare(fakePlugin.commitCount, 0);
    compare(ctx.browser.listView.currentIndex, 1);

    // Clicking a row must not steal keyboard focus from the search
    // field: arrow keys keep navigating (and auditioning)
    keyClick(Qt.Key_Up);
    tryVerify(() => ctx.browser.listView.currentIndex === 0);
    tryVerify(() => fakePlugin.presetIndex === 1);

    // Closing afterwards applies the auditioned selection
    ctx.browser.visible = false;
    compare(fakePlugin.commitCount, 1);
    compare(fakePlugin.revertCount, 0);
  }

  function test_double_click_applies() {
    fakePlugin.reset();
    const ctx = makeBrowser();
    activatedSpy.target = ctx.browser;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();

    ctx.browser.searchField.text = "fac";
    tryCompare(ctx.browser, "count", 2);
    const delegate = ctx.browser.listView.itemAtIndex(1);
    verify(delegate);
    mouseDoubleClickSequence(delegate);
    compare(activatedSpy.count, 1);
    // "Bright" is row 2 in the source model
    compare(activatedSpy.signalArguments[0][0], 2);
    compare(fakePlugin.commitCount, 1);
    compare(fakePlugin.revertCount, 0);
  }

  function test_empty_state_placeholder() {
    fakePlugin.reset();
    const ctx = makeBrowser();
    tryCompare(ctx.browser, "count", 4);
    const emptyLabel = findItem(ctx.browser, "emptyStateLabel");
    verify(emptyLabel);
    verify(!emptyLabel.visible);

    ctx.browser.searchField.text = "zzz";
    tryCompare(ctx.browser, "count", 0);
    tryVerify(() => emptyLabel.visible);
  }

  function test_enter_commits_and_ends_session() {
    fakePlugin.reset();
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    activatedSpy.target = ctx.browser;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();
    hostActivatedSpy.target = ctx.host;
    hostActivatedSpy.signalName = "activatedReceived";
    hostActivatedSpy.clear();

    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    compare(ctx.browser.listView.currentIndex, 0);
    keyClick(Qt.Key_Down);
    tryVerify(() => fakePlugin.presetIndex === 1);

    keyClick(Qt.Key_Return);
    compare(activatedSpy.count, 1);
    compare(activatedSpy.signalArguments[0][0], 1);
    compare(hostActivatedSpy.count, 1);
    compare(hostActivatedSpy.signalArguments[0][0], 1);
    compare(fakePlugin.commitCount, 1);
    compare(fakePlugin.revertCount, 0);
  }

  function test_escape_reverts() {
    fakePlugin.reset();
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    closeSpy.target = ctx.browser;
    closeSpy.signalName = "closeRequested";
    closeSpy.clear();
    hostDismissedSpy.target = ctx.host;
    hostDismissedSpy.signalName = "dismissedReceived";
    hostDismissedSpy.clear();

    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    keyClick(Qt.Key_Down);
    tryVerify(() => fakePlugin.presetIndex === 1);

    keyClick(Qt.Key_Escape);
    compare(fakePlugin.revertCount, 1);
    compare(closeSpy.count, 1);
    compare(hostDismissedSpy.count, 1);
  }

  function test_escape_after_tab_reverts() {
    fakePlugin.reset();
    const model = createTemporaryObject(singleGroupModelComponent, test);
    const host = createTemporaryObject(hostComponent, test);
    const browser = createTemporaryObject(browserComponent, test, {
      "currentIndex": 0,
      "model": model,
      "popupHost": host
    });
    tryVerify(() => browser.searchField.fieldActiveFocus);

    // With the group pane hidden, Tab may move focus to the footer
    // buttons; Escape must revert regardless of where focus sits
    keyClick(Qt.Key_Tab);
    keyClick(Qt.Key_Escape);
    compare(fakePlugin.revertCount, 1);
  }

  function test_filter_clears_highlight() {
    fakePlugin.reset();
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    compare(ctx.browser.listView.currentIndex, 0);

    // Filtering out the highlighted row must clear the highlight:
    // leaving it on a remapped row would audition (and let Enter
    // commit) a preset the user never picked
    ctx.browser.searchField.text = "lead";
    tryCompare(ctx.browser, "count", 1);
    compare(ctx.browser.listView.currentIndex, -1);
  }

  function test_group_pane_filters() {
    fakePlugin.reset();
    const ctx = makeBrowser();
    tryCompare(ctx.browser, "count", 4);
    compare(ctx.browser.groupList.count, 3);
    // Delegates are created on layout, asynchronously
    tryVerify(() => ctx.browser.groupList.itemAtIndex(1) !== null);

    mouseClick(ctx.browser.groupList.itemAtIndex(1));
    tryCompare(ctx.browser, "count", 2);
    mouseClick(ctx.browser.groupList.itemAtIndex(2));
    tryCompare(ctx.browser, "count", 1);
    mouseClick(ctx.browser.groupList.itemAtIndex(0));
    tryCompare(ctx.browser, "count", 4);
  }

  function test_keyboard_group_navigation() {
    fakePlugin.reset();
    const ctx = makeBrowser();
    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    compare(ctx.browser.currentGroup, "");

    // Tab moves focus to the group pane; Down selects the first named
    // group, filtering immediately
    keyClick(Qt.Key_Tab);
    tryVerify(() => ctx.browser.groupList.activeFocus);
    keyClick(Qt.Key_Down);
    tryCompare(ctx.browser, "currentGroup", "Factory");
    tryCompare(ctx.browser, "count", 2);

    // Tab returns to the search field; the filter persists
    keyClick(Qt.Key_Tab);
    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    compare(ctx.browser.count, 2);

    // Escape still reverts from the search field
    keyClick(Qt.Key_Escape);
    compare(fakePlugin.revertCount, 1);
  }

  function test_no_snapshot_mode_escape_does_not_revert() {
    fakePlugin.reset();
    fakePlugin.acceptSessions = false;
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    compare(fakePlugin.beginCount, 1);

    // The plugin refused the session, so Escape must not call revert:
    // there is no snapshot to revert to
    keyClick(Qt.Key_Down);
    keyClick(Qt.Key_Escape);
    compare(fakePlugin.revertCount, 0);
    compare(fakePlugin.commitCount, 0);
  }

  function test_no_snapshot_mode_is_pick_only() {
    fakePlugin.reset();
    fakePlugin.acceptSessions = false;
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    activatedSpy.target = ctx.browser;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();
    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    compare(fakePlugin.beginCount, 1);

    // The plugin refused the session (no state snapshot to revert to):
    // the browser must know, so it never auditions and never calls
    // commit/revert for a session that does not exist
    compare(ctx.browser.pluginSessionActive, false);

    // Navigation alone applies nothing
    keyClick(Qt.Key_Down);
    compare(ctx.browser.listView.currentIndex, 1);
    compare(fakePlugin.presetIndex, -1);

    // Enter still reports the selection (a plain apply needs no
    // session)
    keyClick(Qt.Key_Return);
    compare(activatedSpy.count, 1);
    compare(activatedSpy.signalArguments[0][0], 1);
    compare(fakePlugin.commitCount, 0);
  }

  function test_no_snapshot_mode_passive_close_still_reports() {
    fakePlugin.reset();
    fakePlugin.acceptSessions = false;
    // In-scene (no popupHost): the session follows visibility
    const browser = createTemporaryObject(browserComponent, test, {
      "currentIndex": 0
    });
    appliedSpy.target = browser;
    appliedSpy.signalName = "applied";
    appliedSpy.clear();

    browser.visible = false;
    browser.visible = true;
    compare(fakePlugin.beginCount, 1);
    compare(browser.pluginSessionActive, false);

    // Move the highlight (no audition without a session), then hide:
    // the selection is still reported so the parent applies it
    browser.listView.currentIndex = 1;
    browser.visible = false;
    compare(appliedSpy.count, 1);
    compare(appliedSpy.signalArguments[0][0], 1);
    compare(fakePlugin.commitCount, 0);
  }

  function test_ok_button_applies_and_closes() {
    fakePlugin.reset();
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    activatedSpy.target = ctx.browser;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();
    hostActivatedSpy.target = ctx.host;
    hostActivatedSpy.signalName = "activatedReceived";
    hostActivatedSpy.clear();

    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    keyClick(Qt.Key_Down);
    tryVerify(() => fakePlugin.presetIndex === 1);

    const okButton = findItem(ctx.browser, "okButton");
    verify(okButton);
    mouseClick(okButton);
    compare(activatedSpy.count, 1);
    compare(activatedSpy.signalArguments[0][0], 1);
    compare(hostActivatedSpy.count, 1);
    compare(hostActivatedSpy.signalArguments[0][0], 1);
    compare(fakePlugin.commitCount, 1);
    compare(fakePlugin.revertCount, 0);
  }

  function test_passive_close_within_debounce_window_commits_highlight() {
    fakePlugin.reset();
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);

    // Navigate and hide synchronously, so the 120ms audition debounce
    // cannot fire in between: the pending highlight must still be
    // applied before the session commits, or windowed hosts (which
    // derive their state from the plugin) silently lose it
    keyClick(Qt.Key_Down);
    compare(ctx.browser.listView.currentIndex, 1);
    compare(fakePlugin.presetIndex, -1);
    ctx.browser.visible = false;
    compare(fakePlugin.presetIndex, 1);
    compare(fakePlugin.commitCount, 1);
    compare(fakePlugin.revertCount, 0);
  }

  function test_passive_close_without_navigation_reports_nothing() {
    fakePlugin.reset();
    // In-scene (no popupHost): the session follows visibility
    const browser = createTemporaryObject(browserComponent, test, {
      "currentIndex": 0
    });
    appliedSpy.target = browser;
    appliedSpy.signalName = "applied";
    appliedSpy.clear();

    browser.visible = false;
    browser.visible = true;
    compare(fakePlugin.beginCount, 1);

    // Hidden without any navigation: nothing was auditioned, so there is
    // nothing to report — re-applying the loaded preset would wipe user
    // edits made before the session (setPresetIndex re-applies and
    // clears the dirty flag)
    browser.visible = false;
    compare(appliedSpy.count, 0);
    compare(fakePlugin.commitCount, 1);
    compare(fakePlugin.revertCount, 0);
  }

  function test_passive_hide_commits_selection() {
    fakePlugin.reset();
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    compare(fakePlugin.beginCount, 1);
    hostActivatedSpy.target = ctx.host;
    hostActivatedSpy.signalName = "activatedReceived";
    hostActivatedSpy.clear();
    hostDismissedSpy.target = ctx.host;
    hostDismissedSpy.signalName = "dismissedReceived";
    hostDismissedSpy.clear();

    keyClick(Qt.Key_Down);
    tryVerify(() => fakePlugin.presetIndex === 1);

    // Hidden without an explicit apply/cancel (outside click, host
    // close): the auditioned selection is committed silently (windowed
    // hosts derive their state from the plugin itself)
    ctx.browser.visible = false;
    compare(fakePlugin.commitCount, 1);
    compare(fakePlugin.revertCount, 0);
    compare(hostActivatedSpy.count, 0);
    compare(hostDismissedSpy.count, 0);
  }

  function test_search_enter_commits_first_match() {
    fakePlugin.reset();
    const ctx = makeBrowser();
    activatedSpy.target = ctx.browser;
    activatedSpy.signalName = "activated";
    activatedSpy.clear();

    // Filtering clears the highlight; Enter then commits the first match
    ctx.browser.searchField.text = "lead";
    tryCompare(ctx.browser, "count", 1);
    tryCompare(ctx.browser.listView, "currentIndex", -1);
    keyClick(Qt.Key_Return);
    compare(activatedSpy.count, 1);
    // "Lead" is row 3 in the source model
    compare(activatedSpy.signalArguments[0][0], 3);
    compare(fakePlugin.commitCount, 1);
  }

  function test_search_filters_by_name_and_group() {
    fakePlugin.reset();
    const ctx = makeBrowser();
    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);
    tryCompare(ctx.browser, "count", 4);

    // Matches the group name
    ctx.browser.searchField.text = "fac";
    tryCompare(ctx.browser, "count", 2);

    // Matches the preset name
    ctx.browser.searchField.text = "lead";
    tryCompare(ctx.browser, "count", 1);

    ctx.browser.searchField.text = "";
    tryCompare(ctx.browser, "count", 4);
  }

  function test_session_begins_with_host_and_re_begins_on_show() {
    fakePlugin.reset();
    const ctx = makeBrowser();
    // Windowed hosts create the browser per open: the session begins on
    // creation
    compare(fakePlugin.beginCount, 1);

    ctx.browser.visible = false;
    compare(fakePlugin.commitCount, 1);
    ctx.browser.visible = true;
    compare(fakePlugin.beginCount, 2);
  }

  function test_session_end_request_from_host_commits() {
    fakePlugin.reset();
    const ctx = makeBrowser({
      "currentIndex": 0
    });
    tryVerify(() => ctx.browser.searchField.fieldActiveFocus);

    // Windowed hosts request the session end before tearing down the
    // scene (their deferred destruction can outlive the objects the
    // commit needs): a highlight still inside the debounce window must
    // be applied and committed here, not left for the destruction path
    keyClick(Qt.Key_Down);
    compare(fakePlugin.presetIndex, -1);
    ctx.host.sessionEndRequested();
    compare(fakePlugin.presetIndex, 1);
    compare(fakePlugin.commitCount, 1);
    compare(fakePlugin.revertCount, 0);
  }

  function test_single_group_hides_group_pane() {
    fakePlugin.reset();
    const model = createTemporaryObject(singleGroupModelComponent, test);
    const host = createTemporaryObject(hostComponent, test);
    const browser = createTemporaryObject(browserComponent, test, {
      "model": model,
      "popupHost": host
    });
    tryCompare(browser, "count", 3);

    // With one named group, filtering would be a no-op: the pane is
    // hidden and so are the section headers
    verify(!browser.groupList.visible);
    compare(browser.listView.section.property, "");
  }

  height: 600
  name: "PresetBrowserPopup"
  // Mouse delivery and active focus require the test object to be visible,
  // and TestCase items default to invisible
  visible: true
  when: windowShown
  width: 700

  SignalSpy {
    id: activatedSpy
  }

  SignalSpy {
    id: appliedSpy
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

  // Stand-in for the plugin the browser auditions on (Plugin's session
  // API as plain recordable functions)
  QtObject {
    id: fakePlugin

    // Mirrors Plugin::beginPresetAudition's return: false means no
    // state snapshot was available, so no session exists
    property bool acceptSessions: true
    property int beginCount: 0
    property int commitCount: 0
    property int presetIndex: -1
    property int revertCount: 0

    function beginPresetAudition() {
      beginCount++;
      return acceptSessions;
    }

    function commitPresetAudition() {
      commitCount++;
    }

    function reset() {
      acceptSessions = true;
      presetIndex = -1;
      beginCount = 0;
      commitCount = 0;
      revertCount = 0;
    }

    function revertPresetAudition() {
      revertCount++;
    }
  }

  // Stand-in for the C++ PresetPopupController passed to windowed popups
  Component {
    id: hostComponent

    QtObject {
      signal activatedReceived(int index)
      signal dismissedReceived
      signal sessionEndRequested

      function presetPopupActivated(index) {
        activatedReceived(index);
      }

      function presetPopupDismissed() {
        dismissedReceived();
      }
    }
  }

  // A single named group: the browser hides the group pane and section
  // headers for this case
  Component {
    id: singleGroupModelComponent

    ListModel {
      readonly property var groups: [
        {
          "count": 2,
          "name": "Factory"
        }
      ]
      readonly property bool hasGroups: true
      property var plugin: fakePlugin

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
    }
  }

  Component {
    id: browserComponent

    PresetBrowserPopup {
      height: implicitHeight
      width: implicitWidth

      model: FakePresetModel {
      }
    }
  }

  // ListModel with the PluginPresetListModel surface the browser uses
  component FakePresetModel: ListModel {
    readonly property var groups: [
      {
        "count": 2,
        "name": "Factory"
      },
      {
        "count": 1,
        "name": "User"
      }
    ]
    readonly property bool hasGroups: true
    property var plugin: fakePlugin

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
