// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import ZrythmStyle

TestCase {
  name: "ZrythmTheme"

  // The OKLCH conversion matrices round-trip through sRGB (in-gamut
  // samples; out-of-gamut chroma clamps by design)
  function test_oklch_round_trip() {
    const samples = [
      [0.40, 0.04, 185], [0.40, 0.05, 70], [0.89, 0.03, 300],
      [0.82, 0.10, 145], [0.50, 0.05, 0],
    ];
    for (const [l, c, h] of samples) {
      const rgb = ZrythmTheme.oklchColor(l, c, h);
      const [rl, rc, rh] = ZrythmTheme.rgbToOklch(rgb);
      verify(Math.abs(rl - l) < 0.005, "lightness " + rl + " != " + l);
      verify(Math.abs(rc - c) < 0.008, "chroma " + rc + " != " + c);
      const hueDelta = Math.abs(rh - h) % 360;
      verify(Math.min(hueDelta, 360 - hueDelta) < 0.5,
             "hue " + rh + " != " + h);
    }
  }

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

  // Worked example from DESIGN.md "Clips"
  function test_worked_example_values() {
    compareNear(ZrythmTheme.oklchColor(0.30, 0.065, 185), "#003631");
    compareNear(ZrythmTheme.oklchColor(0.38, 0.065, 185), "#054d47");
    compareNear(ZrythmTheme.oklchColor(0.22, 0.065, 185), "#00211d");
    compareNear(ZrythmTheme.oklchColor(0.60, 0.21, 185), "#009488");
    compareNear(ZrythmTheme.oklchColor(0.30, 0.065, 70), "#422700");
    compareNear(ZrythmTheme.oklchColor(0.38, 0.065, 70), "#583c17");
    compareNear(ZrythmTheme.oklchColor(0.22, 0.065, 70), "#2a1500");
    compareNear(ZrythmTheme.oklchColor(0.60, 0.21, 70), "#b17000");
    compareNear(ZrythmTheme.oklchColor(0.89, 0.06, 185), "#aee8e0");
    compareNear(ZrythmTheme.oklchColor(0.84, 0.06, 185), "#9ed8cf");
    compareNear(ZrythmTheme.oklchColor(0.81, 0.06, 185), "#95cec6");
    compareNear(ZrythmTheme.oklchColor(0.44, 0.18, 185), "#006058");
    compareNear(ZrythmTheme.oklchColor(0.89, 0.06, 70), "#f5d5b0");
    compareNear(ZrythmTheme.oklchColor(0.84, 0.06, 70), "#e4c4a1");
    compareNear(ZrythmTheme.oklchColor(0.81, 0.06, 70), "#dbbb97");
    compareNear(ZrythmTheme.oklchColor(0.44, 0.18, 70), "#744700");
  }

  // The full single-pass API on live sources (dark mode)
  function test_derivation_api() {
    const teal = ZrythmTheme.clipColors("#00A88F", false);
    compareNear(teal.body, "#00372d");
    compareNear(teal.border, "#0d4d42");
    compareNear(teal.scrim, "#00211a");
    compareNear(teal.ink, "#00957f");

    const warm = ZrythmTheme.clipColors("#E08507", false);
    compareNear(warm.body, "#442503");
    compareNear(warm.border, "#5a3a1a");
    compareNear(warm.scrim, "#2c1300");
    compareNear(warm.ink, "#b76b00");

    // the name scrim stays in the hue family, darker than the body
    const scrimSources = ["#E08507", "#00A88F", "#3D6BE8", "#8B5CF6", "#FBD3A5"];
    for (const source of scrimSources) {
      const cc = ZrythmTheme.clipColors(source, false);
      const [bodyL, , bodyH] = ZrythmTheme.rgbToOklch(cc.body);
      const [scrimL, , scrimH] = ZrythmTheme.rgbToOklch(cc.scrim);
      verify(scrimL < bodyL, "scrim lightness " + scrimL + " >= body " + bodyL);
      // the OKLCH round-trip can drift the hue a few degrees at the
      // low recipe chroma (up to ~5° on warm hues)
      verify(Math.abs(scrimH - bodyH) < 6, "scrim hue " + scrimH + " != " + bodyH);
    }
  }

  // The body recipe keeps the source hue at the shared dark-mode
  // lightness
  function test_body_recipe() {
    const orange = "#E08507";
    const orangeBody = ZrythmTheme.clipColors(orange, false).body;
    const [, , oh] = ZrythmTheme.rgbToOklch(orange);
    const [bl, , bh] = ZrythmTheme.rgbToOklch(orangeBody);
    verify(Math.abs(bl - 0.30) < 0.01, "warm lightness " + bl);
    // the OKLCH round-trip can drift the hue a few degrees at the low
    // recipe chroma
    verify(Math.abs(bh - oh) < 4, "warm hue " + bh + " != " + oh);

    const teal = "#00A88F";
    const tealBody = ZrythmTheme.clipColors(teal, false).body;
    const [, , th] = ZrythmTheme.rgbToOklch(teal);
    const [tl, , tbh] = ZrythmTheme.rgbToOklch(tealBody);
    verify(Math.abs(tl - 0.30) < 0.01, "cool lightness " + tl);
    verify(Math.abs(tbh - th) < 4, "cool hue " + tbh + " != " + th);
  }

  function colorsMatch (a: color, b: color): bool {
    return Math.abs(a.r - b.r) <= 1 / 255
      && Math.abs(a.g - b.g) <= 1 / 255
      && Math.abs(a.b - b.b) <= 1 / 255;
  }

  // contentColor and labelColors must stay pinned to the same recipe
  // constants as clipColors: clipColors and the object APIs are
  // parallel implementations of the recipe, so this guard fails if a
  // future change lets them diverge — editors and clip contents
  // render identically only while it holds, for chromatic and
  // achromatic sources alike
  function test_object_colors_match_clip_recipe() {
    const sources = ["#E08507", "#00A88F", "#8B5CF6", "#9A9A9A"];
    for (const source of sources) {
      const cc = ZrythmTheme.clipColors(source, false);
      verify(colorsMatch(ZrythmTheme.contentColor(source, false), cc.ink),
             "content ink mismatch for " + source);
      const label = ZrythmTheme.labelColors(source, false);
      verify(colorsMatch(label.body, cc.body), "label body mismatch for " + source);
      verify(colorsMatch(label.border, cc.border), "label border mismatch for " + source);
    }

    const muted = ZrythmTheme.clipColors("#E08507", true);
    verify(colorsMatch(ZrythmTheme.contentColor("#E08507", true), muted.ink),
           "forced-achromatic ink mismatch");
    verify(colorsMatch(ZrythmTheme.labelColors("#E08507", true).body, muted.body),
           "forced-achromatic body mismatch");
  }

  // Muted clips lose their color entirely: every part renders
  // achromatic at the recipe lightness, whatever the source hue. Gray
  // sources look unchanged (their color was already gone), so the
  // opacity fade is what separates a muted gray clip from a live one.
  function test_muted_palette_is_achromatic() {
    const amberMuted = ZrythmTheme.clipColors("#E08507", true);
    const tealMuted = ZrythmTheme.clipColors("#00A88F", true);
    compareNear(amberMuted.body, "#2e2e2e");
    compareNear(amberMuted.ink, "#808080");
    compareNear(tealMuted.body, "#2e2e2e");
    compareNear(tealMuted.ink, "#808080");

    const amberLive = ZrythmTheme.clipColors("#E08507", false);
    verify(Math.abs(amberMuted.body.r - amberLive.body.r) > 1 / 255
           || Math.abs(amberMuted.body.g - amberLive.body.g) > 1 / 255
           || Math.abs(amberMuted.body.b - amberLive.body.b) > 1 / 255,
           "muted body matches live body");

    const grayLive = ZrythmTheme.clipColors("#5A5A5A", false);
    const grayMuted = ZrythmTheme.clipColors("#5A5A5A", true);
    verify(Math.abs(grayMuted.body.r - grayLive.body.r) <= 1 / 255
           && Math.abs(grayMuted.body.g - grayLive.body.g) <= 1 / 255
           && Math.abs(grayMuted.body.b - grayLive.body.b) <= 1 / 255,
           "muted gray body changed");
    verify(Math.abs(grayMuted.ink.r - grayLive.ink.r) <= 1 / 255
           && Math.abs(grayMuted.ink.g - grayLive.ink.g) <= 1 / 255
           && Math.abs(grayMuted.ink.b - grayLive.ink.b) <= 1 / 255,
           "muted gray ink changed");
  }

  // The content ink holds 3:1 (WCAG graphics contrast) against its
  // body across hues; text on the body follows the polarity rule and
  // holds 4.5:1
  function test_ink_contrast() {
    const sources = ["#E08507", "#00A88F", "#3D6BE8", "#8B5CF6", "#E8459A", "#3DB942", "#323232"];
    for (const source of sources) {
      const cc = ZrythmTheme.clipColors(source, false);
      verify(ZrythmTheme.contrastRatio(cc.ink, cc.body) >= 3.0, "ink contrast for " + source);
      verify(ZrythmTheme.contrastRatio(ZrythmTheme.polarityTextColor(cc.body), cc.body) >= 4.5, "text contrast for " + source);
    }
  }

  // Achromatic sources yield gray surfaces whatever their hue
  function test_achromatic_stays_gray() {
    const cc = ZrythmTheme.clipColors("#5A5A5A", false);
    const [, bodyC] = ZrythmTheme.rgbToOklch(cc.body);
    verify(bodyC < 0.01, "body chroma " + bodyC);
    const [, inkC] = ZrythmTheme.rgbToOklch(cc.ink);
    verify(inkC < 0.01, "ink chroma " + inkC);
  }
}
