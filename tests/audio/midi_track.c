/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-test-config.h"

#include "audio/engine_dummy.h"
#include "audio/midi_event.h"
#include "audio/midi_track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

#include <locale.h>

#define BUFFER_SIZE 20
#define LARGE_BUFFER_SIZE 2000

typedef struct TrackFixture
{
  Track *      midi_track;
  MidiEvents * events;
} TrackFixture;

static void
fixture_set_up (TrackFixture * self)
{
  int      track_pos = TRACKLIST->num_tracks;
  GError * err = NULL;
  tracklist_selections_action_perform_create_midi (
    track_pos, 1, &err);
  g_assert_null (err);
  self->midi_track = TRACKLIST->tracks[track_pos];
  self->events = midi_events_new ();
}

/**
 * Prepares a MIDI region with a note starting at
 * the ZRegion start position and ending at the
 * region end position.
 */
static ZRegion *
prepare_region_with_note_at_start_to_end (
  Track *     track,
  midi_byte_t pitch,
  midi_byte_t velocity)
{
  Position start_pos;
  position_init (&start_pos);
  Position end_pos;
  position_set_to_bar (&end_pos, 3);
  position_add_beats (&end_pos, 1);
  position_add_ticks (&end_pos, 3);
  position_update_frames_from_ticks (&start_pos);
  position_update_frames_from_ticks (&end_pos);
  ZRegion * r = midi_region_new (
    &start_pos, &end_pos,
    track_get_name_hash (track), 0, 0);
  MidiNote * mn1 = midi_note_new (
    &r->id, &start_pos, &end_pos, pitch, velocity);
  midi_region_add_midi_note (r, mn1, 0);

  return r;
}

static void
test_fill_midi_events (void)
{
  test_helper_zrythm_init ();

  TrackFixture   _fixture;
  TrackFixture * fixture = &_fixture;
  fixture_set_up (fixture);

  Track *      track = fixture->midi_track;
  MidiEvents * events = fixture->events;
  MidiEvent *  ev;
  Position     pos;
  position_init (&pos);

  midi_byte_t pitch1 = 35;
  midi_byte_t vel1 = 91;
  ZRegion *   r =
    prepare_region_with_note_at_start_to_end (
      track, pitch1, vel1);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  track_add_region (track, r, NULL, 0, 1, 0);
  MidiNote *       mn = r->midi_notes[0];
  ArrangerObject * mn_obj = (ArrangerObject *) mn;

  /* -- BASIC TESTS (no loops) -- */

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);
  TRANSPORT->play_state = PLAYSTATE_ROLLING;

  /* try basic fill */
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame = (unsigned_frame_t) pos.frames,
    .local_offset = 0,
    .nframes = BUFFER_SIZE,
  };
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_nonnull (ev);
  g_assert_cmpuint (
    midi_get_channel_1_to_16 (ev->raw_buffer), ==,
    midi_region_get_midi_ch (r));
  g_assert_cmpuint (
    midi_get_note_number (ev->raw_buffer), ==,
    pitch1);
  g_assert_cmpuint (
    midi_get_velocity (ev->raw_buffer), ==, vel1);
  g_assert_cmpint ((long) ev->time, ==, pos.frames);
  midi_events_clear (events, F_QUEUED);

  /*
   * Start: region start + 1
   * End: region start + 1 + BUFFER_SIZE
   * Local offset (included above): 1
   *
   * Expected result:
   * MIDI note start is skipped (no events).
   */
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames + 1;
  time_nfo.local_offset = 1;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 0);

  /*
   * Start: region start
   * End: region start + 1
   *
   * Expected result:
   * MIDI note is added even though the cycle is
   * only 1 sample.
   */
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = 1;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 1);
  midi_events_clear (events, F_QUEUED);

  /*
   * Start: region start + BUFFER_SIZE
   * End: region start + BUFFER_SIZE * 2
   *
   * Expected result:
   * No MIDI notes in range.
   */
  position_add_frames (&pos, BUFFER_SIZE);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 0);

  /*
   * Start: region end - (BUFFER_SIZE + 1)
   * End: region end - 1
   *
   * Expected result:
   * No MIDI notes in range just 1 sample before
   * the region end.
   */
  position_set_to_pos (&pos, &r_obj->end_pos);
  position_add_frames (&pos, (-BUFFER_SIZE) - 1);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 0);

  /*
   * Start: region end - BUFFER_SIZE
   * End: region end
   *
   * Expected result:
   * MIDI note off event ends 1 sample before the
   * region end.
   */
  position_set_to_pos (&pos, &r_obj->end_pos);
  position_add_frames (&pos, -BUFFER_SIZE);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 2);
  ev = &events->queued_events[0];
  g_assert_true (midi_is_note_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, BUFFER_SIZE - 1);
  ev = &events->queued_events[1];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, BUFFER_SIZE - 1);
  midi_events_clear (events, F_QUEUED);

  /*
   * Start: midi end - BUFFER_SIZE
   * End: midi end
   *
   * Expected result:
   * Cycle ends on MIDI end so note off is fired
   * at the last sample because the actual MIDI end
   * is exclusive.
   */
  position_add_ticks (&mn_obj->end_pos, -5);
  position_update_frames_from_ticks (
    &mn_obj->end_pos);
  position_set_to_pos (&pos, &mn_obj->end_pos);
  pos.frames = mn_obj->end_pos.frames;
  position_add_frames (&pos, -BUFFER_SIZE);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (ev->time, ==, BUFFER_SIZE - 1);
  position_set_to_pos (
    &mn_obj->end_pos, &r_obj->end_pos);
  midi_events_clear (events, F_QUEUED);

  /*
   * Start: (midi end - BUFFER_SIZE) + 1
   * End: midi end + 1
   *
   * Expected result:
   * MIDI note off event is fired at the frame before
   * its actual end.
   */
  position_add_ticks (&mn_obj->end_pos, -5);
  position_update_frames_from_ticks (
    &mn_obj->end_pos);
  position_set_to_pos (&pos, &mn_obj->end_pos);
  pos.frames = mn_obj->end_pos.frames;
  position_add_frames (&pos, (-BUFFER_SIZE) + 1);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (ev->time, ==, BUFFER_SIZE - 2);
  position_set_to_pos (
    &mn_obj->end_pos, &r_obj->end_pos);
  midi_events_clear (events, F_QUEUED);

  /*
   * Start: region end - (BUFFER_SIZE - 1)
   * End: region end + 1
   *
   * Expected result:
   * MIDI note off event is fired at the frame before
   * the region's actual end.
   */
  position_set_to_pos (&pos, &r_obj->end_pos);
  position_add_frames (&pos, (-BUFFER_SIZE) + 1);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 2);
  ev = &events->queued_events[0];
  g_assert_true (midi_is_note_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, BUFFER_SIZE - 2);
  ev = &events->queued_events[1];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, BUFFER_SIZE - 2);
  midi_events_clear (events, F_QUEUED);

  /*
   * Initialization
   *
   * ZRegion <2.1.1.0 ~ 4.1.1.0>
   *   loop end 2.1.1.0
   * MidiNote <-1.1.1.1>
   */
  position_set_to_bar (&r_obj->pos, 2);
  position_set_to_bar (&r_obj->end_pos, 4);
  position_set_to_bar (&r_obj->loop_end_pos, 2);
  position_update_frames_from_ticks (&r_obj->pos);
  position_update_frames_from_ticks (
    &r_obj->end_pos);
  position_update_frames_from_ticks (
    &r_obj->loop_end_pos);
  position_set_to_bar (&mn_obj->pos, 1);
  position_add_ticks (&mn_obj->pos, -1);
  position_update_frames_from_ticks (&mn_obj->pos);
  position_set_to_pos (
    &mn_obj->end_pos, &mn_obj->pos);
  position_add_ticks (&mn_obj->end_pos, 2000);
  position_update_frames_from_ticks (
    &mn_obj->end_pos);

  /*
   * Premise: note starts 1 tick before region.
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * No MIDI note event.
   */
  position_set_to_pos (&pos, &r_obj->pos);
  position_add_frames (&pos, -365);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = 512;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 0);

  /*
   * Initialization
   *
   * ZRegion <2.1.1.0 ~ 4.1.1.0>
   *   loop start 1.1.1.7
   *   loop end 2.1.1.0
   * MidiNote <1.1.1.7 ~ 2.1.1.7>
   */
  position_set_to_bar (&r_obj->pos, 2);
  position_set_to_bar (&r_obj->end_pos, 4);
  position_set_to_bar (&r_obj->loop_end_pos, 2);
  position_add_ticks (&r_obj->loop_start_pos, 7);
  position_update_frames_from_ticks (&r_obj->pos);
  position_update_frames_from_ticks (
    &r_obj->end_pos);
  position_update_frames_from_ticks (
    &r_obj->loop_start_pos);
  position_update_frames_from_ticks (
    &r_obj->loop_end_pos);
  position_set_to_bar (&mn_obj->pos, 1);
  position_add_ticks (&mn_obj->pos, 7);
  position_set_to_bar (&mn_obj->end_pos, 2);
  position_add_ticks (&mn_obj->end_pos, 7);
  position_update_frames_from_ticks (&mn_obj->pos);
  position_update_frames_from_ticks (
    &mn_obj->end_pos);

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
  position_set_to_bar (&pos, 3);
  position_add_frames (&pos, -365);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = 2000;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 2);
  ev = &events->queued_events[0];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 364);
  ev = &events->queued_events[1];
  g_assert_true (midi_is_note_on (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 365);
  midi_events_clear (events, F_QUEUED);

  /* -- REGION LOOP TESTS -- */

  /*
   * Initialization
   *
   * ZRegion <2.1.1.0 ~ 3.1.1.0>
   *   loop end (same as midi note end)
   * MidiNote <1.1.1.0 ~ (2000 ticks later)>
   */
  position_set_to_bar (&r_obj->pos, 2);
  position_set_to_bar (&r_obj->end_pos, 3);
  position_update_frames_from_ticks (&r_obj->pos);
  position_update_frames_from_ticks (
    &r_obj->end_pos);
  position_set_to_bar (&mn_obj->pos, 1);
  position_update_frames_from_ticks (&mn_obj->pos);
  position_set_to_pos (
    &mn_obj->end_pos, &mn_obj->pos);
  position_add_ticks (&mn_obj->end_pos, 2000);
  position_update_frames_from_ticks (
    &mn_obj->end_pos);
  position_set_to_pos (
    &r_obj->loop_end_pos, &mn_obj->end_pos);

  /*
   * Premise: note starts 1 tick before region.
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * No MIDI note event.
   */
  position_add_ticks (&mn_obj->pos, -1);
  position_update_frames_from_ticks (&mn_obj->pos);
  position_set_to_pos (&pos, &r_obj->pos);
  position_add_frames (&pos, -6);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 0);
  position_add_ticks (&mn_obj->pos, 1);
  position_update_frames_from_ticks (&mn_obj->pos);

  /*
   * Start: way before region start
   * End: way before region start
   *
   * Expected result:
   * No MIDI notes.
   */
  position_set_to_bar (&pos, 1);
  position_update_frames_from_ticks (&pos);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 0);

  /**
   * Start: frame after region end
   * End: BUFFER_SIZE after that way before transport
   * loop.
   *
   * Expected result:
   * No MIDI notes.
   */
  position_set_to_bar (&pos, 3);
  position_update_frames_from_ticks (&pos);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 0);

  /**
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * MIDI note at correct time.
   */
  position_set_to_pos (&pos, &r_obj->pos);
  position_add_frames (&pos, -10);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (ev->time, ==, 10);
  midi_events_clear (events, F_QUEUED);

  /**
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * MIDI note at correct time.
   */
  position_set_to_pos (&pos, &r_obj->pos);
  position_add_frames (&pos, -10);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_cmpuint (ev->time, ==, 10);
  midi_events_clear (events, F_QUEUED);

  /*
   * Initialization
   *
   * ZRegion <2.1.1.0 ~ 3.1.1.0>
   *   loop start <1.1.1.0>
   *   loop end (same as midi note end)
   * MidiNote <1.1.1.0 ~ (2000 ticks later)>
   */
  position_set_to_bar (&r_obj->pos, 2);
  position_set_to_bar (&r_obj->end_pos, 3);
  position_set_to_bar (&r_obj->loop_start_pos, 1);
  position_update_frames_from_ticks (&r_obj->pos);
  position_update_frames_from_ticks (
    &r_obj->end_pos);
  position_set_to_bar (&mn_obj->pos, 1);
  position_update_frames_from_ticks (&mn_obj->pos);
  position_set_to_pos (
    &mn_obj->end_pos, &mn_obj->pos);
  position_add_ticks (&mn_obj->end_pos, 2000);
  position_update_frames_from_ticks (
    &mn_obj->end_pos);
  position_set_to_pos (
    &r_obj->loop_end_pos, &mn_obj->end_pos);

  /**
   * Start: before region loop point
   * End: after region loop point
   *
   * Expected result:
   * MIDI note off at 1 sample before the loop point.
   * MIDI note on at the loop point.
   */
  g_message ("---------");
  position_set_to_pos (&pos, &r_obj->loop_end_pos);
  position_add_frames (&pos, r_obj->pos.frames);
  position_add_frames (&pos, -10);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  g_assert_cmpint (events->num_queued_events, ==, 3);
  ev = &events->queued_events[0];
  g_assert_true (midi_is_note_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 9);
  ev = &events->queued_events[1];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 9);
  ev = &events->queued_events[2];
  g_assert_true (midi_is_note_on (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 10);
  midi_events_clear (events, F_QUEUED);

  /**
   * Premise: note ends on region end (no loops).
   * Start: before region end
   * End: after region end
   *
   * Expected result:
   * 1 MIDI note off, no notes on.
   */
  position_set_to_bar (&r_obj->pos, 2);
  position_set_to_bar (&r_obj->end_pos, 3);
  position_set_to_bar (&r_obj->loop_end_pos, 2);
  position_set_to_bar (&mn_obj->pos, 1);
  position_set_to_bar (&mn_obj->end_pos, 2);
  position_set_to_pos (&pos, &r_obj->end_pos);
  position_add_frames (&pos, -10);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  midi_events_print (events, F_QUEUED);
  g_assert_cmpint (events->num_queued_events, ==, 2);
  ev = &events->queued_events[0];
  g_assert_true (midi_is_note_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 9);
  ev = &events->queued_events[1];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 9);
  midi_events_clear (events, F_QUEUED);

  /**
   * Premise:
   * ZRegion <2.1.1.0 ~ 4.1.1.0>
   *   loop start <1.1.1.0>
   *   clip start <1.3.1.0>
   *   loop end <2.1.1.0>
   * MidiNote <1.1.1.0 ~ (400 frames)>
   * Playhead <3.1.1.0>
   *
   * Expected result:
   * 1 MIDI note on.
   */
  position_set_to_bar (&r_obj->pos, 2);
  position_set_to_bar (&r_obj->end_pos, 4);
  position_set_to_bar (&r_obj->loop_start_pos, 1);
  position_set_to_bar (&r_obj->loop_end_pos, 2);
  position_set_to_bar (&mn_obj->pos, 1);
  position_set_to_bar (&pos, 1);
  position_add_frames (&pos, 400);
  position_set_to_pos (&mn_obj->end_pos, &pos);
  position_set_to_bar (&pos, 3);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = BUFFER_SIZE;
  track_fill_events (track, &time_nfo, events, NULL);
  midi_events_print (events, F_QUEUED);
  g_assert_cmpint (events->num_queued_events, ==, 1);
  ev = &events->queued_events[0];
  g_assert_true (midi_is_note_on (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 0);
  midi_events_clear (events, F_QUEUED);

  /**
   * Premise: note starts at 1.1.1.0 and ends right
   * before transport loop end.
   *
   * Expected result:
   * Note off before loop end and note on at
   * 1.1.1.0.
   */
  position_set_to_bar (&r_obj->pos, 1);
  position_set_to_pos (
    &r_obj->end_pos, &TRANSPORT->loop_end_pos);
  position_set_to_bar (&r_obj->loop_start_pos, 1);
  position_set_to_pos (
    &r_obj->loop_end_pos, &TRANSPORT->loop_end_pos);
  position_set_to_bar (&mn_obj->pos, 1);
  position_set_to_pos (
    &pos, &TRANSPORT->loop_end_pos);
  position_add_frames (&pos, -20);
  position_set_to_pos (&mn_obj->end_pos, &pos);
  position_set_to_pos (
    &pos, &TRANSPORT->loop_end_pos);
  position_add_frames (&pos, -30);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = 30;
  track_fill_events (track, &time_nfo, events, NULL);
  midi_events_print (events, F_QUEUED);
  g_assert_cmpint (events->num_queued_events, ==, 2);
  ev = &events->queued_events[0];
  g_assert_true (midi_is_note_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 9);
  ev = &events->queued_events[1];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 29);
  midi_events_clear (events, F_QUEUED);

  position_set_to_pos (
    &pos, &TRANSPORT->loop_start_pos);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = 10;
  track_fill_events (track, &time_nfo, events, NULL);
  ev = &events->queued_events[0];
  g_assert_true (midi_is_note_on (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 0);
  midi_events_clear (events, F_QUEUED);

  /**
   *
   * Premise: note starts inside region and ends
   *   outside it.
   *
   * Expected result:
   * Note on inside region and note off at region
   * end.
   */
  g_message ("-------------------");
  position_set_to_bar (&r_obj->pos, 1);
  position_set_to_bar (&r_obj->end_pos, 2);
  position_set_to_bar (&r_obj->loop_start_pos, 1);
  position_set_to_bar (&r_obj->loop_end_pos, 2);
  position_set_to_bar (&mn_obj->pos, 2);
  position_add_frames (&mn_obj->pos, -30);
  position_set_to_bar (&mn_obj->end_pos, 3);
  position_set_to_pos (&pos, &mn_obj->pos);
  position_add_frames (&pos, -10);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = 50;
  track_fill_events (track, &time_nfo, events, NULL);
  midi_events_print (events, F_QUEUED);
  g_assert_cmpint (events->num_queued_events, ==, 2);
  ev = &events->queued_events[0];
  g_assert_true (midi_is_note_on (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 10);
  ev = &events->queued_events[1];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 39);
  midi_events_clear (events, F_QUEUED);

  /**
   * Premise: region loops back near the end.
   *
   * Expected result:
   * Note on for a few samples then all notes off.
   */
  g_message ("-------------------");
  position_set_to_bar (&r_obj->pos, 1);
  position_set_to_bar (&r_obj->end_pos, 2);
  position_set_to_bar (&r_obj->loop_start_pos, 1);
  position_set_to_bar (&r_obj->loop_end_pos, 2);
  position_add_frames (&r_obj->loop_end_pos, -10);
  position_set_to_bar (&mn_obj->pos, 1);
  position_set_to_bar (&mn_obj->end_pos, 1);
  position_add_beats (&mn_obj->end_pos, 1);
  position_set_to_pos (&pos, &r_obj->loop_end_pos);
  position_add_frames (&pos, -5);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = 50;
  track_fill_events (track, &time_nfo, events, NULL);
  midi_events_print (events, F_QUEUED);
  g_assert_cmpint (events->num_queued_events, ==, 3);
  ev = &events->queued_events[0];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 4);
  ev = &events->queued_events[1];
  g_assert_true (midi_is_note_on (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 5);
  ev = &events->queued_events[2];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 14);
  midi_events_clear (events, F_QUEUED);

  test_helper_zrythm_cleanup ();
}

#ifdef HAVE_HELM
static void
test_fill_midi_events_from_engine (void)
{
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* create an instrument track for testing */
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  Track * ins_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  ZRegion * r =
    prepare_region_with_note_at_start_to_end (
      ins_track, 35, 60);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  track_add_region (
    ins_track, r, NULL, 0, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);

#  if 0
  MidiNote * mn =
    r->midi_notes[0];
  ArrangerObject * mn_obj =
    (ArrangerObject *) mn;
#  endif

  transport_set_loop (TRANSPORT, true, true);
  position_set_to_pos (
    &TRANSPORT->loop_end_pos, &r_obj->end_pos);
  Position pos;
  position_set_to_pos (&pos, &r_obj->end_pos);
  position_add_frames (&pos, -20);
  transport_set_playhead_pos (TRANSPORT, &pos);
  transport_request_roll (TRANSPORT, true);

  /* run the engine for 1 cycle */
  g_message ("--- processing engine...");
  engine_process (AUDIO_ENGINE, 40);
  g_message ("--- processing recording events...");
  Plugin *     pl = ins_track->channel->instrument;
  Port *       event_in = pl->in_ports[2];
  MidiEvents * midi_events = event_in->midi_events;
  g_assert_cmpint (midi_events->num_events, ==, 3);
  MidiEvent * ev = &midi_events->events[0];
  g_assert_true (midi_is_note_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 19);
  g_assert_cmpuint (
    midi_get_note_number (ev->raw_buffer), ==, 35);
  ev = &midi_events->events[1];
  g_assert_true (
    midi_is_all_notes_off (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 19);
  ev = &midi_events->events[2];
  g_assert_true (midi_is_note_on (ev->raw_buffer));
  g_assert_cmpuint (ev->time, ==, 20);
  g_assert_cmpuint (
    midi_get_note_number (ev->raw_buffer), ==, 35);
  g_assert_cmpuint (
    midi_get_velocity (ev->raw_buffer), ==, 60);

  /* process again and check events are 0 */
  g_message ("--- processing engine...");
  engine_process (AUDIO_ENGINE, 40);
  g_message ("--- processing recording events...");
  g_assert_cmpint (midi_events->num_events, ==, 0);

  test_helper_zrythm_cleanup ();
}
#endif

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/midi_track/"

  g_test_add_func (
    TEST_PREFIX "test fill midi events",
    (GTestFunc) test_fill_midi_events);
#ifdef HAVE_HELM
  g_test_add_func (
    TEST_PREFIX "test fill midi events from engine",
    (GTestFunc) test_fill_midi_events_from_engine);
#endif

  return g_test_run ();
}
