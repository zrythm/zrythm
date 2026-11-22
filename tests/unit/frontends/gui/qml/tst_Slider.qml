// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtTest
import ZrythmStyle

TestCase {
  id: test

  name: "Slider"
  height: 300
  visible: true
  when: windowShown
  width: 400

  Component {
    id: sliderComponent

    Slider {
      x: 10
      y: 10
      width: 200
    }
  }

  Component {
    id: verticalSliderComponent

    Slider {
      height: 200
      orientation: Qt.Vertical
      x: 10
      y: 10
    }
  }

  Component {
    id: rtlSliderComponent

    Slider {
      LayoutMirroring.enabled: true
      x: 10
      y: 10
      width: 200
    }
  }

  FindItem {
    id: findItem
  }

  function createPolishedSlider(component) {
    const control = createTemporaryObject(component, test);
    verify(control);
    verify(waitForItemPolished(control));
    return control;
  }

  function test_track_matches_the_progress_bar_language() {
    const control = createPolishedSlider(sliderComponent);
    // the template (not the Basic fallback) must be in effect
    verify(control instanceof T.Slider);
    compare(control.height, 24);
    compare(control.background.color, control.palette.text);
    compare(control.background.height, 6);
    compare(control.background.radius, 3);
    const fill = findItem.byName(control.background, "fill");
    verify(fill);
    compare(fill.color, control.palette.highlight);
    compare(control.handle.color, control.palette.highlight);
    compare(control.handle.width, 18);
    compare(control.handle.radius, 9);
    compare(control.handle.border.width, 1);
    compare(control.handle.border.color, ZrythmTheme.polarityTextColor(control.palette.highlight));
  }

  function test_fill_and_handle_track_the_value() {
    const control = createPolishedSlider(sliderComponent);
    const fill = findItem.byName(control.background, "fill");
    verify(fill);
    for (const value of [0, 0.25, 0.5, 0.75, 1]) {
      control.value = value;
      compare(fill.width, control.position * control.background.width);
      compare(control.handle.x, control.leftPadding + control.visualPosition * (control.availableWidth - control.handle.width));
      // the handle stays inside the groove at both extremes
      verify(control.handle.x >= control.background.x);
      verify(control.handle.x + control.handle.width <= control.background.x + control.background.width);
    }
  }

  function test_keyboard_focus_shows_accent_ring() {
    const control = createPolishedSlider(sliderComponent);
    control.forceActiveFocus(Qt.TabFocusReason);
    tryVerify(() => control.visualFocus);
    compare(control.handle.border.width, 2);
    compare(control.handle.border.color, control.palette.highlight);
    // non-tab focus keeps the resting ink edge
    control.focus = false;
    tryVerify(() => !control.activeFocus);
    compare(control.handle.border.width, 1);
    compare(control.handle.border.color, ZrythmTheme.polarityTextColor(control.palette.highlight));
  }

  function test_hover_and_press_recolor_the_handle() {
    const control = createPolishedSlider(sliderComponent);
    const centerX = control.handle.x + control.handle.width / 2;
    const centerY = control.height / 2;
    mouseMove(control, centerX, centerY);
    tryCompare(control, "hovered", true);
    tryCompare(control.handle, "color", ZrythmTheme.getColorBlendedTowardsContrast(control.palette.highlight));
    mousePress(control, centerX, centerY, Qt.LeftButton);
    tryCompare(control, "pressed", true);
    tryCompare(control.handle, "color", ZrythmTheme.getStrongerColor(control.palette.highlight));
    mouseRelease(control, centerX, centerY, Qt.LeftButton);
    tryCompare(control, "pressed", false);
    tryCompare(control.handle, "color", ZrythmTheme.getColorBlendedTowardsContrast(control.palette.highlight));
  }

  function test_disabled_control_fades() {
    const control = createPolishedSlider(sliderComponent);
    compare(control.opacity, 1);
    control.enabled = false;
    compare(control.opacity, ZrythmTheme.getOpacity(false, control.Window.active));
  }

  function test_vertical_fill_grows_from_the_bottom() {
    const control = createPolishedSlider(verticalSliderComponent);
    control.value = 0.4;
    const fill = findItem.byName(control.background, "fill");
    verify(fill);
    // the handle sits below the middle: the minimum rests on the bottom edge
    compare(control.handle.y, control.topPadding + control.visualPosition * (control.availableHeight - control.handle.height));
    verify(control.handle.y > control.height / 2);
    // the fill spans from below the middle down to the bottom edge
    compare(fill.y, control.visualPosition * control.background.height);
    compare(fill.y + fill.height, control.background.height);
    compare(fill.height, control.position * control.background.height);
    compare(fill.width, control.background.width);
  }

  function test_rtl_fill_hugs_the_trailing_edge() {
    const control = createPolishedSlider(rtlSliderComponent);
    control.value = 0.3;
    compare(control.visualPosition, 1 - control.position, "visual position is mirrored");
    compare(control.background.scale, -1);
    // the handle measures from the leading (visual right) side
    compare(control.handle.x, control.leftPadding + (1 - control.position) * (control.availableWidth - control.handle.width));
    // the fill keeps covering the value fraction of the groove
    const fill = findItem.byName(control.background, "fill");
    verify(fill);
    compare(fill.width, control.position * control.background.width);
  }
}
