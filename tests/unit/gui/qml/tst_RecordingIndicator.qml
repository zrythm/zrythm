// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import Zrythm

RecordingIndicator {
  id: indicator

  recording: false
  width: 100

  TestCase {
    id: test

    name: "RecordingIndicator"

    function init() {
      indicator.recording = false;
    }

    function test_shows_only_while_recording() {
      compare(indicator.visible, false);
      indicator.recording = true;
      tryVerify(() => indicator.visible);
      indicator.recording = false;
      tryVerify(() => !indicator.visible);
    }

    function test_blink_returns_to_full_opacity() {
      indicator.recording = true;
      tryVerify(() => indicator.visible);
      tryVerify(() => indicator.opacity < 1);
      indicator.recording = false;
      tryCompare(indicator, "opacity", 1);
    }
  }
}
