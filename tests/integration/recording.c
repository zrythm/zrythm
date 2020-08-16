/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/master_track.h"
#include "audio/midi_event.h"
#include "audio/midi_track.h"
#include "audio/recording_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>

#define CYCLE_SIZE 100
#define NOTE_PITCH 56
#define PLAYHEAD_START_BAR 2

/** Constant value to set to the float buffer. */
#define AUDIO_VAL 0.42f

/** Automation value to set. */
#define AUTOMATION_VAL 0.23f

static void
do_takes_no_loop_no_punch (
  Track * ins_track,
  Track * audio_track,
  Track * master_track)
{
  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT);

  /* disable loop & punch */
  transport_set_loop (TRANSPORT, false);
  transport_set_punch_mode_enabled (
    TRANSPORT, false);

  /* move playhead to 2.1.1.0 */
  Position pos;
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  transport_set_playhead_pos (TRANSPORT, &pos);

  /* enable recording for audio & ins track */
  track_set_recording (ins_track, true, false);
  track_set_recording (audio_track, true, false);

  Port * latch_port, * touch_port,
       * on_port;
  AutomationTrack * latch_at, * touch_at, * on_at;

  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input->l->buf[i] = 0.f;
      AUDIO_ENGINE->dummy_input->r->buf[i] = 0.f;
    }

  /* enable recording for master track automation */
  g_assert_cmpint (
    master_track->automation_tracklist.num_ats, >=,
    3);
  latch_at =
    master_track->automation_tracklist.ats[0];
  latch_at->automation_mode = AUTOMATION_MODE_RECORD;
  latch_at->record_mode =
    AUTOMATION_RECORD_MODE_LATCH;
  latch_port =
    port_find_from_identifier (&latch_at->port_id);
  g_assert_nonnull (latch_port);
  float latch_val_at_start =
    latch_port->control;
  touch_at =
    master_track->automation_tracklist.ats[1];
  touch_at->automation_mode = AUTOMATION_MODE_RECORD;
  touch_at->record_mode =
    AUTOMATION_RECORD_MODE_TOUCH;
  touch_port =
    port_find_from_identifier (&touch_at->port_id);
  g_assert_nonnull (touch_port);
  float touch_val_at_start =
    touch_port->control;
  on_at =
    master_track->automation_tracklist.ats[1];
  on_port =
    port_find_from_identifier (&on_at->port_id);
  g_assert_nonnull (on_port);
  float on_val_at_start = on_port->control;

  /* run the engine for 1 cycle */
  engine_process (AUDIO_ENGINE, CYCLE_SIZE);
  recording_manager_process_events (
    RECORDING_MANAGER);

  ZRegion * mr, * audio_r, * latch_r,
          * touch_r, * on_r;
  (void) on_r;
  (void) touch_r;
  ArrangerObject * mr_obj, * audio_r_obj,
                 * latch_r_obj, * touch_r_obj,
                 * on_r_obj;
  (void) on_r_obj;
  (void) touch_r_obj;

  /* assert that MIDI events are created */
  g_assert_cmpint (
    ins_track->lanes[0]->num_regions, ==, 1);
  mr = ins_track->lanes[0]->regions[0];
  mr_obj = (ArrangerObject *) mr;
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &mr_obj->end_pos);
  position_add_frames (&pos, - CYCLE_SIZE);
  g_assert_cmppos (&pos, &mr_obj->pos);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &mr_obj->loop_end_pos);

  /* assert that audio events are created */
  g_assert_cmpint (
    audio_track->lanes[0]->num_regions, ==, 1);
  audio_r = audio_track->lanes[0]->regions[0];
  audio_r_obj = (ArrangerObject *) audio_r;
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &audio_r_obj->end_pos);
  position_add_frames (&pos, - CYCLE_SIZE);
  g_assert_cmppos (&pos, &audio_r_obj->pos);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &audio_r_obj->loop_end_pos);

  /* assert that audio is silent */
  g_assert_cmpuint (
    audio_r->num_frames, ==, CYCLE_SIZE);
  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      g_assert_cmpfloat_with_epsilon (
        audio_r->ch_frames[0][i], 0.f, 0.000001f);
      g_assert_cmpfloat_with_epsilon (
        audio_r->ch_frames[1][i], 0.f, 0.000001f);
    }

  /* assert that automation events are created */
  g_assert_cmpint (latch_at->num_regions, ==, 1);
  latch_r = latch_at->regions[0];
  latch_r_obj = (ArrangerObject *) latch_r;
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &latch_r_obj->end_pos);
  position_add_frames (&pos, - CYCLE_SIZE);
  g_assert_cmppos (&pos, &latch_r_obj->pos);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (
    &pos, &latch_r_obj->loop_end_pos);
  g_assert_cmpint (touch_at->num_regions, ==, 0);
  g_assert_cmpint (on_at->num_regions, ==, 0);
  g_assert_cmpfloat_with_epsilon (
    latch_val_at_start, latch_port->control,
    0.0001f);
  g_assert_cmpfloat_with_epsilon (
    touch_val_at_start, touch_port->control,
    0.0001f);
  g_assert_cmpfloat_with_epsilon (
    on_val_at_start, on_port->control,
    0.0001f);

  /* assert that automation points are created */
  g_assert_cmpint (latch_r->num_aps, ==, 1);
  AutomationPoint * ap = latch_r->aps[0];
  ArrangerObject * ap_obj = (ArrangerObject *) ap;
  position_init (&pos);
  g_assert_cmppos (&pos, &ap_obj->pos);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, latch_val_at_start, 0.0001f);

  /* send a MIDI event */
  Port * port = ins_track->processor->midi_in;
  midi_events_add_note_on (
    port->midi_events, 1, NOTE_PITCH, 70, 0,
    F_QUEUED);
  midi_events_add_note_off (
    port->midi_events, 1, NOTE_PITCH, CYCLE_SIZE - 1,
    F_QUEUED);

  /* send audio data */
  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input->l->buf[i] =
        AUDIO_VAL;
      AUDIO_ENGINE->dummy_input->r->buf[i] =
        - AUDIO_VAL;
    }

  /* send automation data */
  port_set_control_value (
    latch_port, AUTOMATION_VAL, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  /* run the engine */
  engine_process (AUDIO_ENGINE, CYCLE_SIZE);
  recording_manager_process_events (
    RECORDING_MANAGER);

  /* verify MIDI region positions */
  long r_length_frames =
    arranger_object_get_length_in_frames (mr_obj);
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  g_assert_cmppos (&pos, &mr_obj->pos);
  position_init (&pos);
  position_add_frames (&pos, r_length_frames);
  g_assert_cmppos (&pos, &mr_obj->loop_end_pos);
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &mr_obj->end_pos);

  /* verify that MIDI region contains the note at the
   * correct position */
  g_assert_cmpint (mr->num_midi_notes, ==, 1);
  MidiNote * mn = mr->midi_notes[0];
  ArrangerObject * mn_obj = (ArrangerObject *) mn;
  g_assert_cmpint (mn->val, ==, NOTE_PITCH);
  long mn_length_frames =
    arranger_object_get_length_in_frames (mn_obj);
  g_assert_cmpint (
    mn_length_frames, ==, CYCLE_SIZE);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&mn_obj->pos, &pos);

  /* verify audio region positions */
  r_length_frames =
    arranger_object_get_length_in_frames (
      audio_r_obj);
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  g_assert_cmppos (&pos, &audio_r_obj->pos);
  position_init (&pos);
  position_add_frames (&pos, r_length_frames);
  g_assert_cmppos (&pos, &audio_r_obj->loop_end_pos);
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &audio_r_obj->end_pos);

  /* verify audio region contains the correct
   * audio data */
  g_assert_cmpuint (
    audio_r->num_frames, ==, CYCLE_SIZE * 2);
  for (nframes_t i = CYCLE_SIZE;
       i < 2 * CYCLE_SIZE; i++)
    {
      g_assert_cmpfloat_with_epsilon (
        audio_r->ch_frames[0][i], AUDIO_VAL,
        0.000001f);
      g_assert_cmpfloat_with_epsilon (
        audio_r->ch_frames[1][i], - AUDIO_VAL,
        0.000001f);
    }

  /* verify automation region positions */
  r_length_frames =
    arranger_object_get_length_in_frames (
      latch_r_obj);
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  g_assert_cmppos (&pos, &latch_r_obj->pos);
  position_init (&pos);
  position_add_frames (&pos, r_length_frames);
  g_assert_cmppos (
    &pos, &latch_r_obj->loop_end_pos);
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &latch_r_obj->end_pos);

  /* verify automation data is added */
  g_assert_cmpint (latch_r->num_aps, ==, 2);
  ap = latch_r->aps[0];
  ap_obj = (ArrangerObject *) ap;
  position_init (&pos);
  g_assert_cmppos (&pos, &ap_obj->pos);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, latch_val_at_start, 0.0001f);
  ap = latch_r->aps[1];
  ap_obj = (ArrangerObject *) ap;
  position_init (&pos);
  position_add_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &ap_obj->pos);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, AUTOMATION_VAL, 0.0001f);

  /* stop recording */
  track_set_recording (ins_track, false, false);
  track_set_recording (audio_track, false, false);
  latch_at->automation_mode = AUTOMATION_MODE_READ;

  /* run engine 1 more cycle to finalize recording */
  engine_process (AUDIO_ENGINE, CYCLE_SIZE);
  recording_manager_process_events (
    RECORDING_MANAGER);

  /* save and undo/redo */
  test_project_save_and_reload ();
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
}

static void
test_recording_takes (
  Track * ins_track,
  Track * audio_track,
  Track * master_track)
{
  do_takes_no_loop_no_punch (
    ins_track, audio_track, master_track);
}

#ifdef HAVE_HELM
static void
test_recording ()
{
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* create an instrument track for testing */
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      HELM_BUNDLE, HELM_URI, false);

  /* fix the descriptor (for some reason lilv
   * reports it as Plugin instead of Instrument if
   * you don't do lilv_world_load_all) */
  descr->category = PC_INSTRUMENT;
  g_free (descr->category_str);
  descr->category_str =
    plugin_descriptor_category_to_string (
      descr->category);

  /* create an instrument track from helm */
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_INSTRUMENT, descr, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * ins_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* create an audio track */
  ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO, NULL, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* get master track */
  Track * master_track = P_MASTER_TRACK;

  /* create dummy input for audio recording */
  AUDIO_ENGINE->dummy_input =
    stereo_ports_new_generic (
      true, "Dummy input", PORT_OWNER_TYPE_BACKEND,
      AUDIO_ENGINE);

  test_recording_takes (
    ins_track, audio_track, master_track);

  test_helper_zrythm_cleanup ();
}
#endif

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/integration/recording/"

#ifdef HAVE_HELM
  g_test_add_func (
    TEST_PREFIX "test_recording",
    (GTestFunc) test_recording);
#endif

  return g_test_run ();
}
