// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQml

// Shared section state for a row of channel strips: the fold state of
// each section level, and the uniform body height each section takes so
// sections line up across strips. Strips record their current section
// content with updateStrip() whenever it changes and remove themselves
// with forgetStrip() when they are destroyed.
QtObject {
  id: root

  // Fold state of each section level, set by the strips' section
  // headers
  property bool insertsExpanded: true
  property bool midiFxExpanded: true
  property bool sendsExpanded: true

  // Section content of the registered strips, keyed by strip identity
  readonly property var strips: new Map()

  // Uniform section values, reduced across the registered strips by
  // recompute(); strips bind their section bodies to the heights
  property bool anyMidiFx: false
  property real insertsBodyHeight: 0
  property real midiFxBodyHeight: 0
  property real sendsBodyHeight: 0

  // Records one strip's current section content. midiFxContentHeight is
  // 0 on strips without the MIDI FX level; those strips still register
  // so the anyMidiFx reduction sees them.
  function updateStrip(strip: QtObject, supportsMidiFx: bool,
                       midiFxContentHeight: real,
                       insertsContentHeight: real,
                       sendsContentHeight: real): void {
    root.strips.set(strip, {
      supportsMidiFx: supportsMidiFx,
      midiFxContentHeight: midiFxContentHeight,
      insertsContentHeight: insertsContentHeight,
      sendsContentHeight: sendsContentHeight
    });
    root.recompute();
  }

  // Removes a strip's recorded section content
  function forgetStrip(strip: QtObject): void {
    root.strips.delete(strip);
    root.recompute();
  }

  // Reduces the recorded strips into the uniform section values
  function recompute(): void {
    let anyFx = false;
    let fx = 0;
    let ins = 0;
    let snd = 0;
    for (const strip of root.strips.values()) {
      anyFx = anyFx || strip.supportsMidiFx;
      fx = Math.max(fx, strip.midiFxContentHeight);
      ins = Math.max(ins, strip.insertsContentHeight);
      snd = Math.max(snd, strip.sendsContentHeight);
    }
    root.anyMidiFx = anyFx;
    root.midiFxBodyHeight = fx;
    root.insertsBodyHeight = ins;
    root.sendsBodyHeight = snd;
  }
}
