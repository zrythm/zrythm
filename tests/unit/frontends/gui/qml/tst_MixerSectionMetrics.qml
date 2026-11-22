// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtTest
import Zrythm

TestCase {
  id: test

  name: "MixerSectionMetrics"

  Component {
    id: metricsComponent

    MixerSectionMetrics {}
  }

  Component {
    id: stripComponent

    QtObject {}
  }

  property MixerSectionMetrics metrics: null
  property list<QtObject> createdStrips: []

  function init() {
    metrics = metricsComponent.createObject(null);
  }

  function cleanup() {
    for (const strip of createdStrips)
      strip.destroy();
    createdStrips = [];
    metrics.destroy();
    metrics = null;
  }

  function makeStrip(): QtObject {
    const strip = stripComponent.createObject(null);
    createdStrips.push(strip);
    return strip;
  }

  function test_emptyStoreReducesToZero() {
    verify(!metrics.anyMidiFx);
    compare(metrics.insertsBodyHeight, 0);
    compare(metrics.midiFxBodyHeight, 0);
    compare(metrics.sendsBodyHeight, 0);
  }

  function test_uniformHeightsAreTallestAcrossStrips() {
    metrics.updateStrip(makeStrip(), true, 30, 40, 50);
    metrics.updateStrip(makeStrip(), false, 0, 60, 20);
    verify(metrics.anyMidiFx);
    compare(metrics.midiFxBodyHeight, 30);
    compare(metrics.insertsBodyHeight, 60);
    compare(metrics.sendsBodyHeight, 50);
  }

  function test_reductionsFollowLatestReports() {
    metrics.updateStrip(makeStrip(), false, 0, 60, 0);
    metrics.updateStrip(makeStrip(), false, 0, 20, 0);
    compare(metrics.insertsBodyHeight, 60);
    metrics.updateStrip(createdStrips[0], false, 0, 10, 0);
    compare(metrics.insertsBodyHeight, 20);
  }

  function test_forgottenStripDropsOutOfReductions() {
    const a = makeStrip();
    const b = makeStrip();
    metrics.updateStrip(a, true, 30, 40, 50);
    metrics.updateStrip(b, false, 0, 60, 20);
    metrics.forgetStrip(b);
    verify(metrics.anyMidiFx);
    compare(metrics.insertsBodyHeight, 40);
    compare(metrics.sendsBodyHeight, 50);
    metrics.forgetStrip(a);
    verify(!metrics.anyMidiFx);
    compare(metrics.midiFxBodyHeight, 0);
  }
}
