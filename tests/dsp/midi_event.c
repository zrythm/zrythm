// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <stdlib.h>

#include "dsp/midi_event.h"

#include <glib.h>

#include "tests/helpers/zrythm.h"

static void
test_add_note_ons (void)
{
  test_helper_zrythm_init ();

  midi_time_t _time = 402;

  ChordDescriptor * descr = CHORD_EDITOR->chords[0];
  MidiEvents *      events = midi_events_new ();

  /* check at least 3 notes are valid */
  midi_events_add_note_ons_from_chord_descr (
    events, descr, 1, 121, _time, F_NOT_QUEUED);
  for (int i = 0; i < 3; i++)
    {
      MidiEvent * ev = &events->events[i];

      g_assert_cmpuint (ev->time, ==, _time);
      g_assert_true (midi_is_note_on (ev->raw_buffer));
      g_assert_cmpuint (midi_get_velocity (ev->raw_buffer), ==, 121);
    }

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/midi_event/"

  g_test_add_func (
    TEST_PREFIX "test add note ons", (GTestFunc) test_add_note_ons);

  return g_test_run ();
}
