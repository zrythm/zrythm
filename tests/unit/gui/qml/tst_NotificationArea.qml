// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
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
}
