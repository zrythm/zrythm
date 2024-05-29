// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <cstdlib>

#include "dsp/midi_event.h"

#include <glib.h>

#include "tests/helpers/zrythm_helper.h"

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

  midi_events_free (events);

  test_helper_zrythm_cleanup ();
}

static void
test_add_pitchbend (void)
{
  midi_time_t _time = 402;

  MidiEvents * events = midi_events_new ();
  midi_byte_t  buf[3] = { 0xE3, 0x54, 0x39 };

  midi_events_add_event_from_buf (events, _time, buf, 3, F_NOT_QUEUED);
  MidiEvent * ev = &events->events[0];
  g_assert_cmpuint (ev->time, ==, _time);
  g_assert_true (midi_is_pitch_wheel (ev->raw_buffer));
  g_assert_cmpuint (
    midi_get_pitchwheel_value (ev->raw_buffer), ==,
    midi_get_pitchwheel_value (buf));
  g_assert_cmpuint (midi_get_pitchwheel_value (ev->raw_buffer), ==, 0x1CD4);
  g_assert_cmpuint (midi_get_pitchwheel_value (ev->raw_buffer), ==, 7380);
  g_assert_cmpuint (midi_get_pitchwheel_value (ev->raw_buffer), >=, 0);
  g_assert_cmpuint (midi_get_pitchwheel_value (ev->raw_buffer), <, 0x4000);

  midi_events_free (events);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/midi_event/"

  g_test_add_func (
    TEST_PREFIX "test add pitchbend", (GTestFunc) test_add_pitchbend);
  g_test_add_func (
    TEST_PREFIX "test add note ons", (GTestFunc) test_add_note_ons);

  return g_test_run ();
}
