// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import Zrythm

TestCase {
  id: test

  name: "Knob"

  Component {
    id: windowComponent

    ApplicationWindow {
      property alias control: control
      property alias modifiedSpy: modifiedSpy

      Knob {
        id: control

        defaultValue: 0.25
        size: 40

        SignalSpy {
          id: modifiedSpy

          signalName: "valueModified"
          target: control
        }
      }

      height: 200
      visible: true
      width: 200
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

  // Value updates not initiated by the user (undo/redo, automation, MIDI
  // mappings) arrive through the value binding and must not emit
  // valueModified: consumers push undoable edits on that signal, and an
  // echo during undo stack execution corrupts the stack
  function test_programmaticValueChangeDoesNotEmit() {
    window.control.value = 0.7;
    compare(window.control.value, 0.7);
    compare(window.modifiedSpy.count, 0);
  }

  function test_dragEmits() {
    // A horizontal drag of 40px over pixelsForFullRange 250 raises the
    // value from 0 to 0.16
    mouseDrag(window.control, 20, 20, 60, 20, Qt.LeftButton, Qt.NoButton, Qt.NoModifier);
    verify(window.modifiedSpy.count >= 1);
    verify(window.modifiedSpy.signalArguments[0][0] > 0);
  }

  function test_doubleClickResetEmits() {
    mouseDoubleClickSequence(window.control, 20, 20);
    compare(window.modifiedSpy.count, 1);
    verify(Math.abs(window.modifiedSpy.signalArguments[0][0] - 0.25) < 0.0001);
  }
}
