// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import Zrythm

TestCase {
  id: testCase

  name: "ArrangerAutoScroller"

  function init(): void {
    scroller.axes = ArrangerAutoScroller.Axes.Both;
    scroller.editorSettings = settings;
    scroller.maxScrollX = 1000;
    scroller.maxScrollY = 1000;
    scroller.pointer = Qt.point(scroller.viewportWidth / 2, scroller.viewportHeight / 2);
    scroller.pointerValid = true;
    scroller.viewportHeight = 300;
    scroller.viewportWidth = 400;
    settings.x = 100;
    settings.y = 100;
    spy.clear();
  }

  EditorSettings {
    id: settings
  }

  ArrangerAutoScroller {
    id: scroller
  }

  SignalSpy {
    id: spy

    signalName: "scrolled"
    target: scroller
  }

  function test_axesPerAction(): void {
    const both = [
      Arranger.Moving,
      Arranger.MovingCopy,
      Arranger.MovingLink,
      Arranger.CreatingMoving,
      Arranger.CreatingResizingMovingR,
      Arranger.Selecting,
      Arranger.DeleteSelecting,
      Arranger.Ramping,
    ];
    for (const action of both) {
      compare(scroller.axesForArrangerAction(action), ArrangerAutoScroller.Axes.Both);
    }

    const horizontal = [
      Arranger.CreatingResizingR,
      Arranger.ResizingL,
      Arranger.ResizingLLoop,
      Arranger.ResizingLFade,
      Arranger.ResizingR,
      Arranger.ResizingRLoop,
      Arranger.ResizingRFade,
      Arranger.StretchingL,
      Arranger.StretchingR,
      Arranger.Auditioning,
    ];
    for (const action of horizontal) {
      compare(scroller.axesForArrangerAction(action), ArrangerAutoScroller.Axes.Horizontal);
    }

    const vertical = [
      Arranger.ResizingUp,
      Arranger.ResizingUpFadeIn,
      Arranger.ResizingUpFadeOut,
    ];
    for (const action of vertical) {
      compare(scroller.axesForArrangerAction(action), ArrangerAutoScroller.Axes.Vertical);
    }

    const none = [
      Arranger.None,
      Arranger.Panning,
      Arranger.StartingPanning,
      Arranger.Erasing,
      Arranger.StartingErasing,
      Arranger.StartingDeleteSelection,
      Arranger.Cutting,
      Arranger.Renaming,
      Arranger.StartingMoving,
      Arranger.StartingSelection,
      Arranger.ChangingCurve,
    ];
    for (const action of none) {
      compare(scroller.axesForArrangerAction(action), ArrangerAutoScroller.Axes.None);
    }
  }

  function test_rightEdgeScrollsForward(): void {
    scroller.pointer = Qt.point(scroller.viewportWidth - 4, scroller.viewportHeight / 2);
    scroller.tick();
    verify(settings.x > 100, "x scrolled forward: " + settings.x);
    compare(spy.count, 1);
    verify(spy.signalArguments[0][0] > 0);
    compare(spy.signalArguments[0][1], 0);
  }

  function test_leftEdgeScrollsBackwardAndClampsAtStart(): void {
    settings.x = 5;
    scroller.pointer = Qt.point(4, scroller.viewportHeight / 2);
    scroller.tick();
    compare(settings.x, 0);
    compare(spy.count, 1);
    compare(spy.signalArguments[0][0], -5);
    // At the start of the range, another tick changes nothing and emits
    // nothing
    scroller.tick();
    compare(settings.x, 0);
    compare(spy.count, 1);
  }

  function test_verticalEdgesScrollWhenAllowed(): void {
    scroller.pointer = Qt.point(scroller.viewportWidth / 2, scroller.viewportHeight - 2);
    scroller.tick();
    verify(settings.y > 100, "y scrolled down: " + settings.y);
    compare(spy.signalArguments[0][0], 0);
    verify(spy.signalArguments[0][1] > 0);

    settings.y = 3;
    spy.clear();
    scroller.pointer = Qt.point(scroller.viewportWidth / 2, 2);
    scroller.tick();
    compare(settings.y, 0);
    compare(spy.signalArguments[0][1], -3);
  }

  function test_axesRestrictDirection(): void {
    scroller.axes = ArrangerAutoScroller.Axes.Horizontal;
    scroller.pointer = Qt.point(scroller.viewportWidth / 2, scroller.viewportHeight - 2);
    scroller.tick();
    compare(settings.y, 100);
    compare(spy.count, 0);

    scroller.axes = ArrangerAutoScroller.Axes.Vertical;
    scroller.pointer = Qt.point(scroller.viewportWidth - 2, scroller.viewportHeight / 2);
    scroller.tick();
    compare(settings.x, 100);
    compare(spy.count, 0);
  }

  function test_pointerOutsideEdgeZonesDoesNotScroll(): void {
    scroller.tick();
    compare(settings.x, 100);
    compare(settings.y, 100);
    compare(spy.count, 0);
  }

  function test_clampsAtScrollRangeEnd(): void {
    settings.x = scroller.maxScrollX;
    scroller.pointer = Qt.point(scroller.viewportWidth - 2, scroller.viewportHeight / 2);
    scroller.tick();
    compare(settings.x, scroller.maxScrollX);
    compare(spy.count, 0);
  }

  function test_contentSmallerThanViewportNeverScrolls(): void {
    scroller.maxScrollX = 0;
    scroller.maxScrollY = -50;
    scroller.pointer = Qt.point(scroller.viewportWidth - 1, scroller.viewportHeight - 1);
    scroller.tick();
    compare(settings.x, 100);
    compare(settings.y, 100);
    compare(spy.count, 0);
  }

  function test_pointerBeyondEdgeScrollsAtMaxSpeed(): void {
    scroller.pointer = Qt.point(scroller.viewportWidth + 80, scroller.viewportHeight / 2);
    scroller.tick();
    compare(settings.x, 100 + scroller.maxSpeed);
  }

  function test_invalidPointerDoesNothing(): void {
    scroller.pointerValid = false;
    scroller.pointer = Qt.point(scroller.viewportWidth - 1, scroller.viewportHeight / 2);
    scroller.tick();
    compare(settings.x, 100);
    compare(spy.count, 0);
  }

  function test_timerRunsOnlyWhileDragIsActive(): void {
    scroller.active = false;
    verify(!scroller.timer.running);
    scroller.active = true;
    verify(scroller.timer.running);
    scroller.active = false;
    verify(!scroller.timer.running);
  }
}
