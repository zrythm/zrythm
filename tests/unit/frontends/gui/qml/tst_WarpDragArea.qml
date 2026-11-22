// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import Zrythm

TestCase {
  id: test

  name: "WarpDragArea"

  Component {
    id: windowComponent

    ApplicationWindow {
      property alias control: dragArea

      WarpDragArea {
        id: dragArea

        anchors.fill: parent
        defaultValue: 0.25
        from: 0
        to: 1
        value: 0.5

        onValueModified: (newValue) => dragArea.value = newValue
      }

      height: 100
      visible: true
      width: 100
    }
  }

  property ApplicationWindow window: null

  function init() {
    window = windowComponent.createObject(null);
    tryVerify(() => window.visible);
  }

  function cleanup() {
    window.destroy();
    window = null;
  }

  // pinCursor defaults to the application setting; the test harness has no
  // application instance, so the default here is off
  function test_pinCursorOffByDefault() {
    compare(window.control.pinCursor, false);
  }

  function test_dragUpRaisesValueProportionally() {
    mouseDrag(window.control, 50, 50, 0, -50, Qt.LeftButton);
    verify(Math.abs(window.control.value - 0.7) < 1e-6);
  }

  // Explicit moves so every move of the drag carries the modifier; the
  // fine factor must scale each move's contribution
  function test_shiftDragMovesFinely() {
    mousePress(window.control, 50, 50, Qt.LeftButton, Qt.ShiftModifier);
    mouseMove(window.control, 50, 33, 0, Qt.LeftButton, Qt.ShiftModifier);
    mouseMove(window.control, 50, 16, 0, Qt.LeftButton, Qt.ShiftModifier);
    mouseMove(window.control, 50, 0, 0, Qt.LeftButton, Qt.ShiftModifier);
    mouseRelease(window.control, 50, 0, Qt.LeftButton, Qt.ShiftModifier);
    compare(window.control.value, 0.52);
  }

  function test_dragClampsToRange() {
    window.control.value = 0.9;
    mouseDrag(window.control, 50, 50, 0, -50, Qt.LeftButton);
    verify(Math.abs(window.control.value - 1) < 1e-6);
  }

  function test_doubleClickResetsToDefault() {
    window.control.value = 0.8;
    mouseDoubleClickSequence(window.control, 50, 50);
    verify(Math.abs(window.control.value - 0.25) < 1e-6);
  }
}
