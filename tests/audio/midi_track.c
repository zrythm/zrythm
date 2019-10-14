/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine_dummy.h"
#include "audio/midi.h"
#include "audio/midi_track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

#define BUFFER_SIZE 20
#define LARGE_BUFFER_SIZE 2000

typedef struct TrackFixture
{
  Track *      midi_track;
  MidiEvents * events;
} TrackFixture;

static void
fixture_set_up (
  TrackFixture * self)
{
  self->midi_track =
    track_new (
      TRACK_TYPE_MIDI,
      "Test MIDI Track 1",
      F_WITH_LANE);
  Port * port =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "Test Port");
  self->events =
    midi_events_new (port);
}

/**
 * Prepares a MIDI region with a note starting at
 * the Region start position and ending at the
 * region end position.
 */
Region *
prepare_region_with_note_at_start_to_end (
  midi_byte_t pitch,
  midi_byte_t velocity)
{
  Position start_pos = {
    .bars = 1,
    .beats = 1,
    .sixteenths = 1,
    .ticks = 0,
  };
  Position end_pos = {
    .bars = 3,
    .beats = 2,
    .sixteenths = 1,
    .ticks = 3,
  };
  position_update_frames (&start_pos);
  position_update_frames (&end_pos);
  Region * r =
    midi_region_new (
      &start_pos, &end_pos, 1);
  MidiNote * mn1 =
    midi_note_new (
      r, &start_pos, &end_pos, pitch, velocity, 1);
  midi_region_add_midi_note (
    r, mn1);

  return r;
}

static void
test_fill_midi_events ()
{
  TrackFixture _fixture;
  TrackFixture * fixture =&_fixture;
  fixture_set_up (fixture);

  Track * track = fixture->midi_track;
  MidiEvents * events = fixture->events;
  MidiEvent * ev;
  Position pos = {
    .bars = 1,
    .beats = 1,
    .sixteenths = 1,
    .ticks = 0,
  };
  position_update_frames (&pos);

  midi_byte_t pitch1 = 35;
  midi_byte_t vel1= 91;
  Region * r =
    prepare_region_with_note_at_start_to_end (
      pitch1, vel1);
  track_add_region (
    track, r, NULL, 0, 1, 0);
  MidiNote * mn =
    r->midi_notes[0];

  /* -- BASIC TESTS (no loops) -- */

  /* try basic fill */
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_nonnull (ev);
  g_assert_cmpuint (
    ev->channel, ==,
    midi_region_get_midi_ch (r));
  g_assert_cmpuint (
    ev->note_pitch, ==, pitch1);
  g_assert_cmpuint (
    ev->velocity, ==, vel1);
  g_assert_cmpuint (
    ev->time, ==, pos.frames);
  midi_events_clear (events, 1);

  /*
   * Start: region start + 1
   * End: region start + 1 + BUFFER_SIZE
   * Local offset (included above): 1
   *
   * Expected result:
   * MIDI note start is skipped (no events).
   */
  midi_track_fill_midi_events (
    track, pos.frames + 1, 1, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 0);

  /*
   * Start: region start
   * End: region start + 1
   *
   * Expected result:
   * MIDI note is added even though the cycle is
   * only 1 sample.
   */
  midi_track_fill_midi_events (
    track, pos.frames, 0, 1,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  midi_events_clear (events, 1);

  /*
   * Start: region start + BUFFER_SIZE
   * End: region start + BUFFER_SIZE * 2
   *
   * Expected result:
   * No MIDI notes in range.
   */
  position_add_frames (&pos, BUFFER_SIZE);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 0);

  /*
   * Start: region end - (BUFFER_SIZE + 1)
   * End: region end - 1
   *
   * Expected result:
   * No MIDI notes in range just 1 sample before
   * the region end.
   */
  position_set_to_pos (
    &pos, &r->end_pos);
  position_add_frames (&pos, (- BUFFER_SIZE) - 1);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 0);

  /*
   * Start: region end - BUFFER_SIZE
   * End: region end
   *
   * Expected result:
   * MIDI note off event ends 1 sample before the
   * region end.
   */
  position_set_to_pos (
    &pos, &r->end_pos);
  position_add_frames (&pos, - BUFFER_SIZE);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->time, ==, BUFFER_SIZE - 1);
  midi_events_clear (events, 1);

  /*
   * Start: midi end - BUFFER_SIZE
   * End: midi end
   *
   * Expected result:
   * Cycle ends on MIDI end so note off is fired
   * at the last sample because the actual MIDI end
   * is exclusive.
   */
  position_add_ticks (
    &mn->end_pos, -5);
  position_update_frames (&mn->end_pos);
  position_set_to_pos (
    &pos, &mn->end_pos);
  pos.frames = mn->end_pos.frames;
  position_add_frames (&pos, - BUFFER_SIZE);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->time, ==, BUFFER_SIZE - 1);
  position_set_to_pos (
    &mn->end_pos, &r->end_pos);
  midi_events_clear (events, 1);

  /*
   * Start: (midi end - BUFFER_SIZE) + 1
   * End: midi end + 1
   *
   * Expected result:
   * MIDI note off event is fired at the frame before
   * its actual end.
   */
  position_add_ticks (
    &mn->end_pos, -5);
  position_update_frames (&mn->end_pos);
  position_set_to_pos (
    &pos, &mn->end_pos);
  pos.frames = mn->end_pos.frames;
  position_add_frames (&pos, (- BUFFER_SIZE) + 1);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->time, ==, BUFFER_SIZE - 2);
  position_set_to_pos (
    &mn->end_pos, &r->end_pos);
  midi_events_clear (events, 1);

  /*
   * Start: region end - (BUFFER_SIZE - 1)
   * End: region end + 1
   *
   * Expected result:
   * MIDI note off event is fired at the frame before
   * the region's actual end.
   */
  position_set_to_pos (
    &pos, &r->end_pos);
  position_add_frames (&pos, (- BUFFER_SIZE) + 1);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->type, ==, MIDI_EVENT_TYPE_NOTE_OFF);
  g_assert_cmpuint (
    ev->time, ==, BUFFER_SIZE - 2);
  midi_events_clear (events, 1);

  /*
   * Initialization
   *
   * Region <2.1.1.0 ~ 4.1.1.0>
   *   loop end 2.1.1.0
   * MidiNote <-1.1.1.1>
   */
  position_set_to_bar (
    &r->start_pos, 2);
  position_set_to_bar (
    &r->end_pos, 4);
  position_set_to_bar (
    &r->loop_end_pos, 2);
  position_update_frames (&r->start_pos);
  position_update_frames (&r->end_pos);
  position_update_frames (&r->loop_end_pos);
  position_set_to_bar (
    &mn->start_pos, 1);
  position_add_ticks (
    &mn->start_pos, -1);
  position_update_frames (&mn->start_pos);
  position_set_to_pos (
    &mn->end_pos, &mn->start_pos);
  position_add_ticks (
    &mn->end_pos, 2000);
  position_update_frames (&mn->end_pos);

  /*
   * Premise: note starts 1 tick before region.
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * No MIDI note event.
   */
  position_set_to_pos (
    &pos, &r->start_pos);
  position_add_frames (&pos, -365);
  midi_track_fill_midi_events (
    track, pos.frames, 0, 512,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 0);

  /*
   * Initialization
   *
   * Region <2.1.1.0 ~ 4.1.1.0>
   *   loop start 1.1.1.7
   *   loop end 2.1.1.0
   * MidiNote <1.1.1.7 ~ 2.1.1.7>
   */
  position_set_to_bar (
    &r->start_pos, 2);
  position_set_to_bar (
    &r->end_pos, 4);
  position_set_to_bar (
    &r->loop_end_pos, 2);
  position_add_ticks (
    &r->loop_start_pos, 7);
  position_update_frames (&r->start_pos);
  position_update_frames (&r->end_pos);
  position_update_frames (&r->loop_start_pos);
  position_update_frames (&r->loop_end_pos);
  position_set_to_bar (
    &mn->start_pos, 1);
  position_add_ticks (
    &mn->start_pos, 7);
  position_set_to_bar (
    &mn->end_pos, 2);
  position_add_ticks (
    &mn->end_pos, 7);
  position_update_frames (&mn->start_pos);
  position_update_frames (&mn->end_pos);

  /*
   * Premise: note starts at the loop_start point
   * after clip start.
   * Start: before first loop
   * End: after first loop
   *
   * Expected result:
   * MIDI note off at loop point - 1 sample.
   * MIDI note on at loop point.
   */
  position_set_to_bar (
    &pos, 3);
  position_add_frames (&pos, -365);
  midi_track_fill_midi_events (
    track, pos.frames, 0, 2000,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 2);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->type, ==, MIDI_EVENT_TYPE_NOTE_OFF);
  g_assert_cmpuint (
    ev->time, ==, 364);
  ev = &events->queued_events[1];
  g_assert_cmpuint (
    ev->type, ==, MIDI_EVENT_TYPE_NOTE_ON);
  g_assert_cmpuint (
    ev->time, ==, 365);
  midi_events_clear (events, 1);

  /* -- REGION LOOP TESTS -- */

  /*
   * Initialization
   *
   * Region <2.1.1.0 ~ 3.1.1.0>
   *   loop end (same as midi note end)
   * MidiNote <1.1.1.0 ~ (2000 ticks later)>
   */
  position_set_to_bar (
    &r->start_pos, 2);
  position_set_to_bar (
    &r->end_pos, 3);
  position_update_frames (&r->start_pos);
  position_update_frames (&r->end_pos);
  position_set_to_bar (
    &mn->start_pos, 1);
  position_update_frames (&mn->start_pos);
  position_set_to_pos (
    &mn->end_pos, &mn->start_pos);
  position_add_ticks (
    &mn->end_pos, 2000);
  position_update_frames (&mn->end_pos);
  position_set_to_pos (
    &r->loop_end_pos, &mn->end_pos);

  /*
   * Premise: note starts 1 tick before region.
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * No MIDI note event.
   */
  position_add_ticks (
    &mn->start_pos, -1);
  position_update_frames (&mn->start_pos);
  position_set_to_pos (
    &pos, &r->start_pos);
  position_add_frames (&pos, -6);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 0);
  position_add_ticks (
    &mn->start_pos, 1);
  position_update_frames (&mn->start_pos);

  /*
   * Start: way before region start
   * End: way before region start
   *
   * Expected result:
   * No MIDI notes.
   */
  position_set_to_bar (
    &pos, 1);
  position_update_frames (&pos);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 0);

  /**
   * Start: frame after region end
   * End: BUFFER_SIZE after that way before transport
   * loop.
   *
   * Expected result:
   * No MIDI notes.
   */
  position_set_to_bar (
    &pos, 3);
  position_update_frames (&pos);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 0);

  /**
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * MIDI note at correct time.
   */
  position_set_to_pos (
    &pos, &r->start_pos);
  position_add_frames (&pos, - 10);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->time, ==, 10);
  midi_events_clear (events, 1);

  /**
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * MIDI note at correct time.
   */
  position_set_to_pos (
    &pos, &r->start_pos);
  position_add_frames (&pos, - 10);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->time, ==, 10);
  midi_events_clear (events, 1);

  /*
   * Initialization
   *
   * Region <2.1.1.0 ~ 3.1.1.0>
   *   loop start <1.1.1.0>
   *   loop end (same as midi note end)
   * MidiNote <1.1.1.0 ~ (2000 ticks later)>
   */
  position_set_to_bar (
    &r->start_pos, 2);
  position_set_to_bar (
    &r->end_pos, 3);
  position_set_to_bar (
    &r->loop_start_pos, 1);
  position_update_frames (&r->start_pos);
  position_update_frames (&r->end_pos);
  position_set_to_bar (
    &mn->start_pos, 1);
  position_update_frames (&mn->start_pos);
  position_set_to_pos (
    &mn->end_pos, &mn->start_pos);
  position_add_ticks (
    &mn->end_pos, 2000);
  position_update_frames (&mn->end_pos);
  position_set_to_pos (
    &r->loop_end_pos, &mn->end_pos);

  /**
   * Start: before region loop point
   * End: after region loop point
   *
   * Expected result:
   * MIDI note off at 1 sample before the loop point.
   * MIDI note on at the loop point.
   */
  position_set_to_pos (
    &pos, &r->loop_end_pos);
  position_add_frames (&pos, r->start_pos.frames);
  position_add_frames (&pos, - 10);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  g_assert_cmpint (
    events->num_queued_events, ==, 2);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->type, ==, MIDI_EVENT_TYPE_NOTE_OFF);
  g_assert_cmpuint (
    ev->time, ==, 9);
  ev = &events->queued_events[1];
  g_assert_cmpuint (
    ev->type, ==, MIDI_EVENT_TYPE_NOTE_ON);
  g_assert_cmpuint (
    ev->time, ==, 10);
  midi_events_clear (events, 1);

  /**
   * Premise: note ends on region end (no loops).
   * Start: before region end
   * End: after region end
   *
   * Expected result:
   * 1 MIDI note off, no notes on.
   */
  position_set_to_bar (
    &r->start_pos, 2);
  position_set_to_bar (
    &r->end_pos, 3);
  position_set_to_bar (
    &r->loop_end_pos, 2);
  position_set_to_bar (
    &mn->start_pos, 1);
  position_set_to_bar (
    &mn->end_pos, 2);
  position_set_to_pos (
    &pos, &r->end_pos);
  position_add_frames (&pos, - 10);
  midi_track_fill_midi_events (
    track, pos.frames, 0, BUFFER_SIZE,
    events);
  midi_events_print (events, 1);
  g_assert_cmpint (
    events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (
    ev->type, ==, MIDI_EVENT_TYPE_NOTE_OFF);
  g_assert_cmpuint (
    ev->time, ==, 9);
  midi_events_clear (events, 1);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/midi_track/"

  g_test_add_func (
    TEST_PREFIX "test fill midi events",
    (GTestFunc) test_fill_midi_events);

  return g_test_run ();
}
