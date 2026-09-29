// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtTest
import QmlTests

TestCase {
  id: test

  name: "NotificationCenterButton"
  height: 300
  visible: true
  when: windowShown
  width: 400

  function makeButton(): NotificationCenterButton {
    const center = createTemporaryObject(centerComponent, test);
    return createTemporaryObject(buttonComponent, test, { "notificationCenter": center });
  }

  function findButton(item: Item, label: string): Button {
    if (item instanceof Button && item.text === label)
      return item;
    for (let i = 0; i < item.children.length; ++i) {
      const found = findButton(item.children[i], label);
      if (found !== null)
        return found;
    }
    return null;
  }

  function test_badgeHiddenWithoutWarningsOrErrors() {
    const button = makeButton();
    button.notificationCenter.postInfo("Autosaved", "");
    button.notificationCenter.postSuccess("Exported", "");
    compare(button.badgeVisible, false);
    compare(button.badgeText, String(0));
  }

  function test_badgeCountsUnacknowledgedWarningsAndErrors() {
    const button = makeButton();
    button.notificationCenter.postInfo("Autosaved", "");
    button.notificationCenter.postWarning("Plugin Failed to Load", "");
    button.notificationCenter.postError("Cannot Perform Operation", "");
    compare(button.badgeVisible, true);
    compare(button.badgeText, "2");
  }

  function test_badgeCapsAtNinePlus() {
    const button = makeButton();
    for (let i = 0; i < 12; ++i)
      button.notificationCenter.postWarning("Failure " + i, "");
    compare(button.badgeVisible, true);
    compare(button.badgeText, "9+");
  }

  function test_coalescedOccurrencesBadgePerOccurrence() {
    const button = makeButton();
    button.notificationCenter.postError("Device Disconnected", "");
    button.notificationCenter.postError("Device Disconnected", "");
    compare(button.badgeText, "2");
  }

  function test_criticalNeverBadgesButIsListed() {
    const button = makeButton();
    button.notificationCenter.postCritical("Project Loading Failed", "");
    compare(button.badgeVisible, false);
    compare(button.notificationCenter.unacknowledgedCriticals().length, 1);
  }

  function test_openingPopoverAcknowledgesEverything() {
    const button = makeButton();
    button.notificationCenter.postWarning("Plugin Failed to Load", "");
    button.notificationCenter.postError("Cannot Perform Operation", "");
    compare(button.badgeVisible, true);

    button.popover.open();
    tryCompare(button.popover, "visible", true);
    tryCompare(button, "badgeVisible", false);
  }

  function test_eventsArrivingWhilePopoverIsOpenAreSeenOnClose() {
    const button = makeButton();
    button.popover.open();
    tryCompare(button.popover, "visible", true);

    button.notificationCenter.postWarning("Plugin Failed to Load", "");
    compare(button.badgeVisible, true);

    button.popover.close();
    tryCompare(button.popover, "visible", false);
    tryCompare(button, "badgeVisible", false);
  }

  function test_clearAllEmptiesTheHistory() {
    const button = makeButton();
    button.notificationCenter.postError("Cannot Perform Operation", "");
    button.notificationCenter.postCritical("Project Loading Failed", "");

    button.popover.open();
    tryCompare(button.popover, "visible", true);

    const clearAll = findButton(button.popover.contentItem, "Clear All");
    verify(clearAll !== null);
    clearAll.clicked();

    compare(button.notificationCenter.unacknowledgedCriticals().length, 0);
    compare(button.badgeVisible, false);
  }

  function test_relativeTimestampFormat() {
    const button = makeButton();
    // Reference times come from the popover's own clock so the expected
    // labels do not depend on the delay since its creation
    const reference = button.popover.nowMs;
    compare(
      button.popover.relativeTimestamp(new Date(reference)), "just now");
    compare(
      button.popover.relativeTimestamp(new Date(reference - 45 * 1000)),
      "45 s ago");
    compare(
      button.popover.relativeTimestamp(new Date(reference - 5 * 60000)),
      "5 min ago");
    compare(
      button.popover.relativeTimestamp(new Date(reference - 3 * 3600000)),
      "3 h ago");
  }

  function makeArea(center: NotificationCenter): NotificationArea {
    return createTemporaryObject(areaComponent, test, { "notificationCenter": center });
  }

  function test_dismissingToastAcknowledgesIt() {
    const center = createTemporaryObject(centerComponent, test);
    const button = createTemporaryObject(buttonComponent, test, { "notificationCenter": center });
    const area = makeArea(center);
    verify(area !== null);

    center.postError("Cannot Perform Operation", "");
    compare(button.badgeVisible, true);
    compare(area.visibleCount, 1);

    area.dismissTopmost();
    tryCompare(area, "visibleCount", 0);
    tryCompare(button, "badgeVisible", false);
  }

  function test_dismissingCoalescedToastAcknowledgesEveryOccurrence() {
    const center = createTemporaryObject(centerComponent, test);
    const button = createTemporaryObject(buttonComponent, test, { "notificationCenter": center });
    const area = makeArea(center);

    center.postError("Device Disconnected", "");
    center.postError("Device Disconnected", "");
    center.postError("Device Disconnected", "");
    compare(area.topmostCount(), 3);
    compare(button.badgeText, "3");

    area.dismissTopmost();
    tryCompare(button, "badgeVisible", false);
  }

  function test_expiredToastStaysUnacknowledged() {
    const center = createTemporaryObject(centerComponent, test);
    const button = createTemporaryObject(buttonComponent, test, { "notificationCenter": center });
    const area = createTemporaryObject(
      areaComponent, test,
      { "notificationCenter": center, "warningDurationMs": 100 });

    center.postWarning("Plugin Failed to Load", "");
    tryCompare(area, "visibleCount", 0);

    compare(button.badgeVisible, true);
    compare(button.badgeText, "1");
  }

  Component {
    id: buttonComponent

    NotificationCenterButton {
    }
  }

  Component {
    id: centerComponent

    NotificationCenter {
    }
  }

  Component {
    id: areaComponent

    NotificationArea {
    }
  }
}
