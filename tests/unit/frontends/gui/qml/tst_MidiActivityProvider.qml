// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtTest
import QmlTests
import Zrythm

TestCase {
  id: test

  name: "MidiActivityProvider"
  when: windowShown

  function test_event_kind_flags_support_bitwise_filtering(): void {
    const noteOn = MidiActivityProvider.NoteOn;
    const noteOff = MidiActivityProvider.NoteOff;
    const allNotesOff = MidiActivityProvider.AllNotesOff;
    const controlChange = MidiActivityProvider.ControlChange;

    verify(noteOn !== 0);
    verify(noteOff !== 0);
    verify(allNotesOff !== 0);

    // each kind occupies its own bit
    compare(noteOn & controlChange, 0);
    compare(noteOff & controlChange, 0);
    compare(allNotesOff & controlChange, 0);

    // consumers can build a mask and intersect a batch with it
    const noteMask = noteOn | noteOff | allNotesOff;
    compare(noteMask & allNotesOff, allNotesOff);
    compare(noteMask & controlChange, 0);
  }
}
