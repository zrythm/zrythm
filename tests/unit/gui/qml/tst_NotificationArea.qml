// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtTest
import QmlTests

TestCase {
  id: test

  name: "NotificationArea"
  height: 300
  visible: true
  when: windowShown
  width: 400

  function makeArea(properties: var): NotificationArea {
    const center = createTemporaryObject(centerComponent, test);
    return createTemporaryObject(areaComponent, test, Object.assign({ "notificationCenter": center }, properties ?? {}));
  }

  function findActionButton(item: Item, label: string): T.Button {
    for (let i = 0; i < item.children.length; ++i) {
      const child = item.children[i];
      if (child instanceof T.Button && child.text === label)
        return child;
      const nested = findActionButton(child, label);
      if (nested !== null)
        return nested;
    }
    return null;
  }

  function test_visibleToastsCappedAtThree() {
    const area = makeArea({ "infoDurationMs": 0 });
    for (let i = 0; i < 5; ++i)
      area.notificationCenter.postInfo("toast " + i, "");
    compare(area.visibleCount, 3);
    compare(area.pendingCount, 2);
  }

  function test_visibleToastCoalescesBySeverityAndTitle() {
    const area = makeArea({});
    area.notificationCenter.postWarning("Saved", "");
    area.notificationCenter.postWarning("Saved", "");
    area.notificationCenter.postInfo("Saved", "");
    compare(area.visibleCount, 2);
    compare(area.pendingCount, 0);
  }

  function test_coalescedCountCoversActiveToastsOnly() {
    const area = makeArea({});
    area.notificationCenter.postError("Repeats", "");
    area.notificationCenter.postError("Repeats", "");
    compare(area.visibleCount, 1);
    compare(area.topmostCount(), 2);
    area.dismissTopmost();
    tryCompare(area, "visibleCount", 0);
    area.notificationCenter.postError("Repeats", "");
    compare(area.visibleCount, 1);
    compare(area.topmostCount(), 1);
  }

  function test_queuedEventCoalescesIntoQueuedToast() {
    const area = makeArea({ "infoDurationMs": 0 });
    for (let i = 0; i < 3; ++i)
      area.notificationCenter.postInfo("filler " + i, "");
    area.notificationCenter.postInfo("queued", "");
    area.notificationCenter.postInfo("queued", "");
    compare(area.visibleCount, 3);
    compare(area.pendingCount, 1);
  }

  function test_criticalNeverToasts() {
    const area = makeArea({});
    const spy = createTemporaryObject(criticalSpyComponent, test, { "target": area });
    area.notificationCenter.postCritical("Project Loading Failed", "");
    compare(spy.count, 1);
    compare(area.visibleCount, 0);
    compare(area.pendingCount, 0);
  }

  function test_infoToastAutoDismisses() {
    const area = makeArea({ "infoDurationMs": 100 });
    area.notificationCenter.postInfo("transient", "");
    compare(area.visibleCount, 1);
    tryCompare(area, "visibleCount", 0);
  }

  function test_errorToastHoldsItsSlotWithinItsLifetime() {
    // Shrink every other duration and wait past all of them: an Error
    // toast that wrongly picked up any timer would hide inside the wait
    // window
    const area = makeArea({ "infoDurationMs": 100, "successDurationMs": 100, "warningDurationMs": 100 });
    area.notificationCenter.postError("stays", "");
    wait(1000);
    compare(area.visibleCount, 1);
  }

  function test_errorToastAutoDismissesAfterItsDuration() {
    const area = makeArea({ "errorDurationMs": 100 });
    area.notificationCenter.postError("transient", "");
    compare(area.visibleCount, 1);
    tryCompare(area, "visibleCount", 0);
  }

  function test_stackedErrorToastHoldsItsSlotWithinItsLifetime() {
    const area = makeArea({ "infoDurationMs": 100, "successDurationMs": 100, "warningDurationMs": 100 });
    area.notificationCenter.postError("Repeats", "");
    area.notificationCenter.postError("Repeats", "");
    area.notificationCenter.postError("Repeats", "");
    compare(area.topmostCount(), 3);
    wait(1000);
    compare(area.visibleCount, 1);
    compare(area.topmostCount(), 3);
  }

  function test_dismissalPromotesQueuedToast() {
    const area = makeArea({ "infoDurationMs": 0 });
    for (let i = 0; i < 5; ++i)
      area.notificationCenter.postInfo("toast " + i, "");
    compare(area.visibleCount, 3);
    area.dismissTopmost();
    tryCompare(area, "pendingCount", 1);
    compare(area.visibleCount, 3);
  }

  function test_coalescedEventRestartsDismissTimer() {
    const area = makeArea({ "warningDurationMs": 500 });
    area.notificationCenter.postWarning("Flapping", "");
    wait(250);
    // Halfway through the original 500 ms deadline: stacking restarts the
    // timer, so the toast must outlive the original deadline
    area.notificationCenter.postWarning("Flapping", "");
    compare(area.topmostCount(), 2);
    wait(300);
    compare(area.visibleCount, 1);
    tryCompare(area, "visibleCount", 0);
  }

  function test_criticalPostedBeforeCreationIsPresentedOnCreation() {
    const center = createTemporaryObject(centerComponent, test);
    center.postCritical("Audio Device Initialization Failed", "No device");
    const area = createTemporaryObject(
      areaComponent, test, { "notificationCenter": center });

    // Presenting the critical notification acknowledged it, and no
    // toast was created for it
    compare(center.unacknowledgedCriticals().length, 0);
    compare(area.visibleCount, 0);
  }

  function test_actionButtonClickInvokesCallbackAndDismissesToast() {
    const area = makeArea({ "errorDurationMs": 60000 });
    const state = createTemporaryObject(actionStateComponent, test);
    area.notificationCenter.postError(
      "Save Failed", "detail", "", "Do It",
      () => { state.fired = true; });
    compare(area.visibleCount, 1);
    const button = findActionButton(area, "Do It");
    verify(button !== null);
    verify(button.visible);
    button.clicked(null);
    verify(state.fired);
    tryCompare(area, "visibleCount", 0);
  }

  function test_actionCallbackIsReadableAsFunction() {
    const area = makeArea({});
    const state = createTemporaryObject(actionStateComponent, test);
    const spy = createTemporaryObject(
      postedSpyComponent, test, { "target": area.notificationCenter });
    area.notificationCenter.postError(
      "Save Failed", "detail", "", "Do It",
      () => { state.fired = true; });
    const notification = spy.signalArguments[0][0];
    const callback = notification.actionCallback;
    verify(typeof callback === "function");
    callback();
    verify(state.fired);
  }

  function test_coalescedToastTriggersNewestOccurrenceAction() {
    const area = makeArea({ "errorDurationMs": 60000 });
    const state = createTemporaryObject(actionStateComponent, test);
    area.notificationCenter.postError(
      "Flapping", "", "", "Do It",
      () => { state.first = true; });
    area.notificationCenter.postError(
      "Flapping", "", "", "Do It",
      () => { state.second = true; });
    compare(area.visibleCount, 1);
    compare(area.topmostCount(), 2);
    const button = findActionButton(area, "Do It");
    button.clicked(null);
    verify(!state.first);
    verify(state.second);
    tryCompare(area, "visibleCount", 0);
  }

  // The button presents the newest occurrence's label, consistent with
  // the callback it triggers
  function test_coalescedToastShowsNewestOccurrenceLabel() {
    const area = makeArea({ "errorDurationMs": 60000 });
    const state = createTemporaryObject(actionStateComponent, test);
    area.notificationCenter.postError(
      "Flapping", "", "tag", "Retry",
      () => { state.first = true; });
    area.notificationCenter.postError(
      "Flapping", "", "tag", "Save As…",
      () => { state.second = true; });
    compare(area.visibleCount, 1);
    verify(findActionButton(area, "Retry") === null);
    const button = findActionButton(area, "Save As…");
    verify(button !== null);
    button.clicked(null);
    verify(!state.first);
    verify(state.second);
    tryCompare(area, "visibleCount", 0);
  }

  // Writing a property of a destroyed captured object is skipped
  // without throwing, and the rest of the callback still runs
  function test_actionCallbackToleratesDestroyedCapturedProperty() {
    const area = makeArea({ "errorDurationMs": 60000 });
    const state = createTemporaryObject(actionStateComponent, test);
    const victim = Qt.createQmlObject(
      "import QtQuick; Item { property bool marker: false }", test);
    area.notificationCenter.postError(
      "Save Failed", "detail", "", "Do It",
      () => { victim.marker = true; state.survived = true; });
    victim.destroy();
    // destroy() defers deletion to the event loop; a destroyed item's
    // parent reads as undefined (it had one while alive)
    tryVerify(() => victim.parent === undefined);
    const button = findActionButton(area, "Do It");
    button.clicked(null);
    verify(state.survived);
    tryCompare(area, "visibleCount", 0);
  }

  // Calling a method of a destroyed captured object throws inside the
  // callback; triggerAction() reports it and nothing crashes
  function test_actionCallbackToleratesDestroyedCapturedMethod() {
    const area = makeArea({ "errorDurationMs": 60000 });
    const victim = Qt.createQmlObject(
      "import QtQuick; Item { function poke(): void {} }", test);
    area.notificationCenter.postError(
      "Save Failed", "detail", "", "Do It",
      () => { victim.poke(); });
    victim.destroy();
    tryVerify(() => victim.parent === undefined);
    const button = findActionButton(area, "Do It");
    button.clicked(null);
    tryCompare(area, "visibleCount", 0);
  }

  function test_triggerActionWithoutCallbackDoesNothing() {
    const area = makeArea({ "errorDurationMs": 60000 });
    const spy = createTemporaryObject(
      postedSpyComponent, test, { "target": area.notificationCenter });
    area.notificationCenter.postError("Plain", "");
    const notification = spy.signalArguments[0][0];
    verify(notification.actionCallback === undefined);
    notification.triggerAction();
    compare(area.visibleCount, 1);
  }

  Component {
    id: areaComponent

    NotificationArea {
    }
  }

  Component {
    id: centerComponent

    NotificationCenter {
    }
  }

  Component {
    id: criticalSpyComponent

    SignalSpy {
      signalName: "criticalNotification"
    }
  }

  Component {
    id: actionStateComponent

    QtObject {
      property bool fired: false
      property bool survived: false
      property bool first: false
      property bool second: false
    }
  }

  Component {
    id: postedSpyComponent

    SignalSpy {
      signalName: "notificationPosted"
    }
  }
}
