// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import QmlTests

TestCase {
  id: test

  function test_getLoadColor_boundaries() {
    var indicator = createTemporaryObject(dspComp, test);
    compare(indicator.getLoadColor(49), indicator.lowLoadColor);
    compare(indicator.getLoadColor(50), indicator.mediumLoadColor);
    compare(indicator.getLoadColor(74), indicator.mediumLoadColor);
    compare(indicator.getLoadColor(75), indicator.highLoadColor);
    compare(indicator.getLoadColor(89), indicator.highLoadColor);
    compare(indicator.getLoadColor(90), indicator.criticalLoadColor);
  }

  name: "DspLoadIndicator"
  when: windowShown

  Component {
    id: dspComp

    DspLoadIndicator {
    }
  }
}
