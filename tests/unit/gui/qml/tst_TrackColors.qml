// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import Zrythm
import ZrythmStyle

TestCase {
  name: "TrackColors"

  // The pinned default palette: one hue family per track type, dark and
  // light variants (derivation in gui/qquick/track_colors.h)
  readonly property var pins: [
    { type: Track.Master, dark: "#ff5799", light: "#f00580" },
    { type: Track.MidiGroup, dark: "#ff682b", light: "#e24e00" },
    { type: Track.Instrument, dark: "#e68100", light: "#bc6800" },
    { type: Track.Midi, dark: "#cd9200", light: "#a77600" },
    { type: Track.MidiBus, dark: "#b49f00", light: "#928100" },
    { type: Track.Audio, dark: "#76b400", light: "#5a8b00" },
    { type: Track.AudioBus, dark: "#00bd6c", light: "#009253" },
    { type: Track.AudioGroup, dark: "#00b8a1", light: "#008e7d" },
    { type: Track.Folder, dark: "#00b2c8", light: "#008a9b" },
    { type: Track.Chord, dark: "#3fa3ff", light: "#0083e1" },
    { type: Track.Modulator, dark: "#9f88ff", light: "#8a62ff" },
    { type: Track.Marker, dark: "#df59fc", light: "#c43be0" },
  ]

  // Compares with a ±1/255 tolerance: QColor quantizes through
  // 16-bit channels
  function compareNear(actualColor: color, expectedHex: string): void {
    const expected = Qt.rgba(
      parseInt(expectedHex.substr(1, 2), 16) / 255,
      parseInt(expectedHex.substr(3, 2), 16) / 255,
      parseInt(expectedHex.substr(5, 2), 16) / 255, 1);
    verify(Math.abs(actualColor.r - expected.r) <= 1 / 255
           && Math.abs(actualColor.g - expected.g) <= 1 / 255
           && Math.abs(actualColor.b - expected.b) <= 1 / 255,
           actualColor + " != " + expectedHex);
  }

  function test_dark_defaults(): void {
    for (const pin of pins) {
      compareNear(TrackColors.defaultColor(pin.type, true), pin.dark);
    }
  }

  function test_light_defaults(): void {
    for (const pin of pins) {
      compareNear(TrackColors.defaultColor(pin.type, false), pin.light);
    }
  }

  function test_light_defaults_meet_contrast_target(): void {
    for (const pin of pins) {
      verify(
        ColorMath.contrastRatio(
          TrackColors.defaultColor(pin.type, false), ZrythmTheme.lightPageColor) >= 3.0,
        "type " + pin.type + " light default below 3:1 contrast");
    }
  }

  // Every track type appears once in the canonical picker order, and
  // that order follows the hue wheel
  function test_canonical_types_cover_all_types_in_hue_order(): void {
    compare(TrackColors.canonicalTypes().length, pins.length);
    let previousHue = -1;
    for (let i = 0; i < TrackColors.canonicalTypes().length; i++) {
      const hue = TrackColors.hueOf(TrackColors.canonicalTypes()[i]);
      verify(hue >= previousHue, "canonical type " + i + " out of hue order");
      previousHue = hue;
    }
  }
}
