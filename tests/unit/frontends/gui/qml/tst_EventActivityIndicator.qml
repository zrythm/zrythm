// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import QmlTests
import ZrythmStyle

TestCase {
  id: test

  name: "EventActivityIndicator"
  when: windowShown

  Component {
    id: indicatorComp

    EventActivityIndicator {
      height: ZrythmTheme.compactControlHeight
    }
  }

  function createIndicator(): EventActivityIndicator {
    return createTemporaryObject(indicatorComp, test);
  }

  function test_geometry() {
    const indicator = createIndicator();
    compare(indicator.width, 4);
    compare(indicator.height, 18);

    const well = findChild(indicator, "idleWell");
    verify(well);
    compare(well.width, 4);
    compare(well.height, 4);
    compare(well.radius, 2);
    compare(well.x, 0);
    compare(well.y, 7);

    const overlay = findChild(indicator, "activeOverlay");
    verify(overlay);
    compare(overlay.width, 4);
    compare(overlay.height, 4);
    compare(overlay.x, 0);
    compare(overlay.y, 7);
  }

  function test_colors() {
    const indicator = createIndicator();
    const well = findChild(indicator, "idleWell");
    const overlay = findChild(indicator, "activeOverlay");
    const originalDarkMode = ZrythmTheme.darkMode;
    const originalPrimaryColor = ZrythmTheme.primaryColor;

    const verifyColors = () => {
      const activeColor = ZrythmTheme.indicatorColor(ZrythmTheme.primaryColor);
      compare(well.color, ZrythmTheme.buttonBackgroundColor);
      compare(well.border.color, ZrythmTheme.colorPalette.mid);
      compare(overlay.color, activeColor);
      verify(ZrythmTheme.contrastRatio(activeColor, ZrythmTheme.pageColor) >= 3.0,
             "active dot contrast too low");
    };

    ZrythmTheme.darkMode = true;
    ZrythmTheme.primaryColor = ZrythmTheme.zrythmColor;
    verifyColors();
    ZrythmTheme.darkMode = false;
    ZrythmTheme.primaryColor = ZrythmTheme.celestialBlueColor;
    verifyColors();

    ZrythmTheme.primaryColor = originalPrimaryColor;
    ZrythmTheme.darkMode = originalDarkMode;
  }

  function test_decorative_for_accessibility() {
    const indicator = createIndicator();
    verify(indicator.Accessible.ignored);
    verify(!indicator.activeFocus);
  }

  function test_pulse_has_immediate_onset_and_hold() {
    const indicator = createIndicator();
    const overlay = findChild(indicator, "activeOverlay");
    compare(overlay.opacity, 0);
    verify(!indicator.active);

    indicator.pulse();
    // no fade-in: fully lit on the same frame
    compare(overlay.opacity, 1);
    verify(indicator.active);

    // still lit just before the hold expires
    wait(indicator.holdDurationMs - 30);
    compare(overlay.opacity, 1);

    // releases after the hold and goes fully dark
    tryVerify(() => !indicator.active, 10 * indicator.releaseDurationMs);
    tryCompare(overlay, "opacity", 0, 10 * indicator.releaseDurationMs);
  }

  function test_release_fades_over_release_duration() {
    const indicator = createIndicator();
    const overlay = findChild(indicator, "activeOverlay");

    indicator.pulse();
    // the hold has expired and the release animation is under way
    tryVerify(() => overlay.opacity < 1 && overlay.opacity > 0, 10 * indicator.holdDurationMs);
    tryVerify(() => overlay.opacity === 0, 10 * indicator.releaseDurationMs);
  }

  function test_pulse_during_hold_restarts_hold() {
    const indicator = createIndicator();
    const overlay = findChild(indicator, "activeOverlay");

    indicator.pulse();
    wait(indicator.holdDurationMs - 30);
    // a second pulse restarts the hold window
    indicator.pulse();
    wait(indicator.holdDurationMs - 30);
    compare(overlay.opacity, 1);
    verify(indicator.active);

    tryCompare(overlay, "opacity", 0, 10 * indicator.releaseDurationMs);
  }

  function test_pulse_during_release_relights_immediately() {
    const indicator = createIndicator();
    const overlay = findChild(indicator, "activeOverlay");

    indicator.pulse();
    // catch the release mid-fade
    tryVerify(() => overlay.opacity < 1 && overlay.opacity > 0, 10 * indicator.holdDurationMs);

    indicator.pulse();
    compare(overlay.opacity, 1);
    verify(indicator.active);

    tryCompare(overlay, "opacity", 0, 10 * indicator.releaseDurationMs);
  }

  function test_reset_extinguishes_immediately() {
    const indicator = createIndicator();
    const overlay = findChild(indicator, "activeOverlay");

    indicator.pulse();
    indicator.reset();
    compare(overlay.opacity, 0);
    verify(!indicator.active);

    // stays dark without further pulses
    wait(indicator.holdDurationMs + indicator.releaseDurationMs);
    compare(overlay.opacity, 0);
  }

  Component {
    id: trackMetersComp

    TrackMeters {
      audioEngine: null
      channel: null
      portObservationManager: null
      track: null
    }
  }

  // TrackMeters only presents output meters. Without a channel its exposed
  // values fall back to zero instead of erroring on the inactive loaders.
  function test_track_meters_without_channel_have_zero_values() {
    const meters = createTemporaryObject(trackMetersComp, test);
    verify(meters);
    compare(meters.currentAmplitude, 0);
    compare(meters.currentPeak, 0);
    verify(!findChild(meters, "eventIndicatorLoader"));
  }

  // The generic MIDI meter defaults on for the mixer and inspector; track
  // headers suppress it when their activity dot presents the same events.
  function test_track_meters_midi_meter_can_be_suppressed() {
    const meters = createTemporaryObject(trackMetersComp, test);
    verify(meters.showMidiMeter);
    meters.showMidiMeter = false;
    verify(!meters.showMidiMeter);
    compare(meters.currentAmplitude, 0);
    compare(meters.currentPeak, 0);
  }
}
