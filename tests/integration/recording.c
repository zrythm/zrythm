// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
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

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#define CYCLE_SIZE 100
#define NOTE_PITCH 56
#define PLAYHEAD_START_BAR 2

/** Constant value to set to the float buffer. */
#define AUDIO_VAL 0.42f

/** Automation value to set. */
#define AUTOMATION_VAL 0.23f

#define SET_CACHES_AND_PROCESS \
  engine_set_run (AUDIO_ENGINE, 0); \
  tracklist_set_caches ( \
    TRACKLIST, CACHE_TYPE_PLAYBACK_SNAPSHOTS); \
  engine_set_run (AUDIO_ENGINE, 1); \
  engine_process (AUDIO_ENGINE, CYCLE_SIZE)

static void
prepare (void)
{
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create dummy input for audio recording */
  AUDIO_ENGINE->dummy_input = stereo_ports_new_generic (
    true, "Dummy input", "dummy_input",
    PORT_OWNER_TYPE_AUDIO_ENGINE, AUDIO_ENGINE);
  port_allocate_bufs (AUDIO_ENGINE->dummy_input->l);
  port_allocate_bufs (AUDIO_ENGINE->dummy_input->r);

  /* sleep for a bit because Port's last_change interferes
   * with touch automation recording if it's too close to the
   * current time */
  g_usleep (1000000); // 1 sec
}

static void
do_takes_no_loop_no_punch (
  Track * ins_track,
  Track * audio_track,
  Track * master_track)
{
  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT, true);

  /* disable loop & punch */
  transport_set_loop (TRANSPORT, false, true);
  transport_set_punch_mode_enabled (TRANSPORT, false);

  /* move playhead to 2.1.1.0 */
  transport_set_playhead_to_bar (
    TRANSPORT, PLAYHEAD_START_BAR);

  /* enable recording for audio & ins track */
  track_set_recording (ins_track, true, false);
  track_set_recording (audio_track, true, false);

  Port *           latch_port, *touch_port, *on_port;
  AutomationTrack *latch_at, *touch_at, *on_at;

  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input->l->buf[i] = 0.f;
      AUDIO_ENGINE->dummy_input->r->buf[i] = 0.f;
    }

  /* enable recording for master track automation */
  g_assert_cmpint (
    master_track->automation_tracklist.num_ats, >=, 3);
  latch_at = master_track->automation_tracklist.ats[0];
  latch_at->record_mode = AUTOMATION_RECORD_MODE_LATCH;
  automation_track_set_automation_mode (
    latch_at, AUTOMATION_MODE_RECORD, F_NO_PUBLISH_EVENTS);
  latch_port = port_find_from_identifier (&latch_at->port_id);
  g_assert_nonnull (latch_port);
  float latch_val_at_start = latch_port->control;
  touch_at = master_track->automation_tracklist.ats[1];
  touch_at->record_mode = AUTOMATION_RECORD_MODE_TOUCH;
  automation_track_set_automation_mode (
    touch_at, AUTOMATION_MODE_RECORD, F_NO_PUBLISH_EVENTS);
  touch_port = port_find_from_identifier (&touch_at->port_id);
  g_assert_nonnull (touch_port);
  float touch_val_at_start = touch_port->control;
  on_at = master_track->automation_tracklist.ats[1];
  on_port = port_find_from_identifier (&on_at->port_id);
  g_assert_nonnull (on_port);
  float on_val_at_start = on_port->control;

  SET_CACHES_AND_PROCESS;
  recording_manager_process_events (RECORDING_MANAGER);

  g_message ("process 1 ended");

  ZRegion *mr, *audio_r, *latch_r, *touch_r, *on_r;
  (void) on_r;
  (void) touch_r;
  ArrangerObject *mr_obj, *audio_r_obj, *latch_r_obj,
    *touch_r_obj, *on_r_obj;
  (void) on_r_obj;
  (void) touch_r_obj;

  /* assert that MIDI events are created */
  g_assert_cmpint (ins_track->lanes[0]->num_regions, ==, 1);
  mr = ins_track->lanes[0]->regions[0];
  mr_obj = (ArrangerObject *) mr;
  Position pos;
  position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &mr_obj->end_pos);
  position_add_frames (&pos, -CYCLE_SIZE);
  g_assert_cmppos (&pos, &mr_obj->pos);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &mr_obj->loop_end_pos);

  /* assert that audio events are created */
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 1);
  audio_r = audio_track->lanes[0]->regions[0];
  audio_r_obj = (ArrangerObject *) audio_r;
  position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &audio_r_obj->end_pos);
  position_add_frames (&pos, -CYCLE_SIZE);
  g_assert_cmppos (&pos, &audio_r_obj->pos);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &audio_r_obj->loop_end_pos);

  /* assert that audio is silent */
  AudioClip * clip = audio_region_get_clip (audio_r);
  g_assert_cmpuint (clip->num_frames, ==, CYCLE_SIZE);
  for (int i = 0; i < 2; i++)
    {
      g_assert_true (audio_frames_empty (
        &clip->ch_frames[i][0], CYCLE_SIZE));
    }

  /* assert that automation events are created */
  g_assert_cmpint (latch_at->num_regions, ==, 1);
  latch_r = latch_at->regions[0];
  latch_r_obj = (ArrangerObject *) latch_r;
  position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &latch_r_obj->end_pos);
  position_add_frames (&pos, -CYCLE_SIZE);
  g_assert_cmppos (&pos, &latch_r_obj->pos);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &latch_r_obj->loop_end_pos);
  g_assert_cmpint (touch_at->num_regions, ==, 0);
  g_assert_cmpint (on_at->num_regions, ==, 0);
  g_assert_cmpfloat_with_epsilon (
    latch_val_at_start, latch_port->control, 0.0001f);
  g_assert_cmpfloat_with_epsilon (
    touch_val_at_start, touch_port->control, 0.0001f);
  g_assert_cmpfloat_with_epsilon (
    on_val_at_start, on_port->control, 0.0001f);

  /* assert that automation points are created */
  g_assert_cmpint (latch_r->num_aps, ==, 1);
  AutomationPoint * ap = latch_r->aps[0];
  ArrangerObject *  ap_obj = (ArrangerObject *) ap;
  position_init (&pos);
  g_assert_cmppos (&pos, &ap_obj->pos);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, latch_val_at_start, 0.0001f);

  /* send a MIDI event */
  Port * port = ins_track->processor->midi_in;
  midi_events_add_note_on (
    port->midi_events, 1, NOTE_PITCH, 70, 0, F_QUEUED);
  midi_events_add_note_off (
    port->midi_events, 1, NOTE_PITCH, CYCLE_SIZE - 1,
    F_QUEUED);

  /* send audio data */
  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input->l->buf[i] = AUDIO_VAL;
      AUDIO_ENGINE->dummy_input->r->buf[i] = -AUDIO_VAL;
    }

  /* send automation data */
  port_set_control_value (
    latch_port, AUTOMATION_VAL, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  /* run the engine */
  SET_CACHES_AND_PROCESS;
  recording_manager_process_events (RECORDING_MANAGER);

  g_message ("process 2 ended");

  /* verify MIDI region positions */
  long r_length_frames =
    arranger_object_get_length_in_frames (mr_obj);
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  g_assert_cmppos (&pos, &mr_obj->pos);
  position_init (&pos);
  position_add_frames (&pos, r_length_frames);
  g_assert_cmppos (&pos, &mr_obj->loop_end_pos);
  position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &mr_obj->end_pos);

  /* verify that MIDI region contains the note at the
   * correct position */
  g_assert_cmpint (mr->num_midi_notes, ==, 1);
  MidiNote *       mn = mr->midi_notes[0];
  ArrangerObject * mn_obj = (ArrangerObject *) mn;
  g_assert_cmpint (mn->val, ==, NOTE_PITCH);
  long mn_length_frames =
    arranger_object_get_length_in_frames (mn_obj);
  g_assert_cmpint (mn_length_frames, ==, CYCLE_SIZE);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&mn_obj->pos, &pos);

  /* verify audio region positions */
  r_length_frames =
    arranger_object_get_length_in_frames (audio_r_obj);
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  g_assert_cmppos (&pos, &audio_r_obj->pos);
  position_init (&pos);
  position_add_frames (&pos, r_length_frames);
  g_assert_cmppos (&pos, &audio_r_obj->loop_end_pos);
  position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &audio_r_obj->end_pos);

  /* verify audio region contains the correct
   * audio data */
  clip = audio_region_get_clip (audio_r);
  g_assert_cmpuint (clip->num_frames, ==, CYCLE_SIZE * 2);
  for (nframes_t i = CYCLE_SIZE; i < 2 * CYCLE_SIZE; i++)
    {
      g_assert_cmpfloat_with_epsilon (
        clip->ch_frames[0][i], AUDIO_VAL, 0.000001f);
      g_assert_cmpfloat_with_epsilon (
        clip->ch_frames[1][i], -AUDIO_VAL, 0.000001f);
    }

  /* verify automation region positions */
  r_length_frames =
    arranger_object_get_length_in_frames (latch_r_obj);
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  g_assert_cmppos (&pos, &latch_r_obj->pos);
  position_init (&pos);
  position_add_frames (&pos, r_length_frames);
  g_assert_cmppos (&pos, &latch_r_obj->loop_end_pos);
  position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
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
  SET_CACHES_AND_PROCESS;
  transport_request_pause (TRANSPORT, true);
  recording_manager_process_events (RECORDING_MANAGER);

  g_assert_cmpint (
    RECORDING_MANAGER->num_active_recordings, ==, 0);

  g_message ("process 3 ended");

  /* save and undo/redo */
  test_project_save_and_reload ();
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
}

static void
do_takes_loop_no_punch (
  Track * ins_track,
  Track * audio_track,
  Track * master_track)
{
  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT, true);

  /* enable loop & disable punch */
  transport_set_loop (TRANSPORT, true, true);
  position_set_to_bar (&TRANSPORT->loop_end_pos, 5);
  transport_set_punch_mode_enabled (TRANSPORT, false);

#define FRAMES_BEFORE_LOOP 4

  /* move playhead to 4 ticks before loop */
  Position pos;
  position_set_to_pos (&pos, &TRANSPORT->loop_end_pos);
  position_add_frames (&pos, -FRAMES_BEFORE_LOOP);
  transport_set_playhead_pos (TRANSPORT, &pos);

  /* enable recording for audio & ins track */
  track_set_recording (ins_track, true, false);
  track_set_recording (audio_track, true, false);

  Port *           latch_port, *touch_port, *on_port;
  AutomationTrack *latch_at, *touch_at, *on_at;

  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input->l->buf[i] = 0.f;
      AUDIO_ENGINE->dummy_input->r->buf[i] = 0.f;
    }

  /* enable recording for master track automation */
  g_assert_cmpint (
    master_track->automation_tracklist.num_ats, >=, 3);
  latch_at = master_track->automation_tracklist.ats[0];
  latch_at->record_mode = AUTOMATION_RECORD_MODE_LATCH;
  automation_track_set_automation_mode (
    latch_at, AUTOMATION_MODE_RECORD, F_NO_PUBLISH_EVENTS);
  latch_port = port_find_from_identifier (&latch_at->port_id);
  g_assert_nonnull (latch_port);
  float latch_val_at_start = latch_port->control;
  touch_at = master_track->automation_tracklist.ats[1];
  touch_at->record_mode = AUTOMATION_RECORD_MODE_TOUCH;
  automation_track_set_automation_mode (
    touch_at, AUTOMATION_MODE_RECORD, F_NO_PUBLISH_EVENTS);
  touch_port = port_find_from_identifier (&touch_at->port_id);
  g_assert_nonnull (touch_port);
  float touch_val_at_start = touch_port->control;
  on_at = master_track->automation_tracklist.ats[1];
  on_port = port_find_from_identifier (&on_at->port_id);
  g_assert_nonnull (on_port);
  float on_val_at_start = on_port->control;

  /* run the engine for 1 cycle */
  g_message ("--- processing engine...");
  SET_CACHES_AND_PROCESS;
  g_message ("--- processing recording events...");
  recording_manager_process_events (RECORDING_MANAGER);

  ZRegion *mr, *audio_r, *latch_r, *touch_r, *on_r;
  (void) on_r;
  (void) touch_r;
  ArrangerObject *mr_obj, *audio_r_obj, *latch_r_obj,
    *touch_r_obj, *on_r_obj;
  (void) on_r_obj;
  (void) touch_r_obj;

  /* assert that MIDI events are created */
  /* FIXME this assumes it runs after the first test
   */
  g_assert_cmpint (ins_track->lanes[1]->num_regions, ==, 1);
  g_assert_cmpint (ins_track->lanes[2]->num_regions, ==, 1);
  mr = ins_track->lanes[1]->regions[0];
  mr_obj = (ArrangerObject *) mr;
  position_set_to_pos (&pos, &TRANSPORT->loop_end_pos);
  g_assert_cmppos (&pos, &mr_obj->end_pos);
  position_add_frames (&pos, -FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &mr_obj->pos);
  position_from_frames (&pos, FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &mr_obj->loop_end_pos);
  mr = ins_track->lanes[2]->regions[0];
  mr_obj = (ArrangerObject *) mr;
  position_set_to_pos (&pos, &TRANSPORT->loop_start_pos);
  g_assert_cmppos (&pos, &mr_obj->pos);
  position_add_frames (&pos, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &mr_obj->end_pos);
  position_from_frames (&pos, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &mr_obj->loop_end_pos);

  /* assert that audio events are created */
  g_assert_cmpint (audio_track->lanes[1]->num_regions, ==, 1);
  g_assert_cmpint (audio_track->lanes[2]->num_regions, ==, 1);
  audio_r = audio_track->lanes[1]->regions[0];
  audio_r_obj = (ArrangerObject *) audio_r;
  position_set_to_pos (&pos, &TRANSPORT->loop_end_pos);
  g_assert_cmppos (&pos, &audio_r_obj->end_pos);
  position_add_frames (&pos, -FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &audio_r_obj->pos);
  position_from_frames (&pos, FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &audio_r_obj->loop_end_pos);
  /* assert that audio is silent */
  AudioClip * clip = audio_region_get_clip (audio_r);
  g_assert_cmpuint (clip->num_frames, ==, FRAMES_BEFORE_LOOP);
  for (nframes_t i = 0; i < (nframes_t) FRAMES_BEFORE_LOOP; i++)
    {
      g_assert_cmpfloat_with_epsilon (
        clip->ch_frames[0][i], 0.f, 0.000001f);
      g_assert_cmpfloat_with_epsilon (
        clip->ch_frames[1][i], 0.f, 0.000001f);
    }
  audio_r = audio_track->lanes[2]->regions[0];
  audio_r_obj = (ArrangerObject *) audio_r;
  position_set_to_pos (&pos, &TRANSPORT->loop_start_pos);
  g_assert_cmppos (&pos, &audio_r_obj->pos);
  position_add_frames (&pos, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &audio_r_obj->end_pos);
  position_from_frames (&pos, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &audio_r_obj->loop_end_pos);
  /* assert that audio is silent */
  clip = audio_region_get_clip (audio_r);
  g_assert_cmpuint (
    clip->num_frames, ==, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  for (nframes_t i = 0;
       i < CYCLE_SIZE - (nframes_t) FRAMES_BEFORE_LOOP; i++)
    {
      g_assert_cmpfloat_with_epsilon (
        clip->ch_frames[0][i], 0.f, 0.000001f);
      g_assert_cmpfloat_with_epsilon (
        clip->ch_frames[1][i], 0.f, 0.000001f);
    }

  /* assert that automation events are created */
  g_assert_cmpint (latch_at->num_regions, ==, 2);
  latch_r = latch_at->regions[0];
  latch_r_obj = (ArrangerObject *) latch_r;
  position_set_to_pos (&pos, &TRANSPORT->loop_end_pos);
  g_assert_cmppos (&pos, &latch_r_obj->end_pos);
  position_add_frames (&pos, -FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &latch_r_obj->pos);
  position_from_frames (&pos, FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &latch_r_obj->loop_end_pos);
  latch_r = latch_at->regions[1];
  latch_r_obj = (ArrangerObject *) latch_r;
  position_set_to_pos (&pos, &TRANSPORT->loop_start_pos);
  g_assert_cmppos (&pos, &latch_r_obj->pos);
  position_add_frames (&pos, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &latch_r_obj->end_pos);
  position_from_frames (&pos, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &latch_r_obj->loop_end_pos);

  g_assert_cmpint (touch_at->num_regions, ==, 0);
  g_assert_cmpint (on_at->num_regions, ==, 0);
  g_assert_cmpfloat_with_epsilon (
    latch_val_at_start, latch_port->control, 0.0001f);
  g_assert_cmpfloat_with_epsilon (
    touch_val_at_start, touch_port->control, 0.0001f);
  g_assert_cmpfloat_with_epsilon (
    on_val_at_start, on_port->control, 0.0001f);

  /* assert that automation points are created */
  latch_r = latch_at->regions[0];
  latch_r_obj = (ArrangerObject *) latch_r;
  g_assert_cmpint (latch_r->num_aps, ==, 1);
  AutomationPoint * ap = latch_r->aps[0];
  ArrangerObject *  ap_obj = (ArrangerObject *) ap;
  position_init (&pos);
  g_assert_cmppos (&pos, &ap_obj->pos);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, latch_val_at_start, 0.0001f);
#if 0
  ap = latch_r->aps[1];
  ap_obj = (ArrangerObject *) ap;
  position_from_frames (&pos, FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &ap_obj->pos);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, latch_val_at_start, 0.0001f);
#endif

  /* TODO try the above steps again with data like
   * below */
#if 0
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
  SET_CACHES_AND_PROCESS;
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
  SET_CACHES_AND_PROCESS;
  recording_manager_process_events (
    RECORDING_MANAGER);
#endif

#undef FRAMES_BEFORE_LOOP

  /* save and undo/redo */
  test_project_save_and_reload ();
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
}

static void
test_recording_takes (
  Track * ins_track,
  Track * audio_track,
  Track * master_track)
{
  int ins_track_pos = ins_track->pos;
  int audio_track_pos = audio_track->pos;
  int master_track_pos = master_track->pos;
  g_assert_cmpint (audio_track_pos, <, TRACKLIST->num_tracks);

  prepare ();
  do_takes_no_loop_no_punch (
    ins_track, audio_track, master_track);

  g_assert_cmpint (audio_track_pos, <, TRACKLIST->num_tracks);
  ins_track = TRACKLIST->tracks[ins_track_pos];
  audio_track = TRACKLIST->tracks[audio_track_pos];
  master_track = TRACKLIST->tracks[master_track_pos];

  prepare ();
  do_takes_loop_no_punch (
    ins_track, audio_track, master_track);
}

static void
test_recording (void)
{
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an instrument track for testing */
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);
  Track * ins_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* create an audio track */
  Track * audio_track =
    track_create_empty_with_action (TRACK_TYPE_AUDIO, NULL);
  g_assert_true (IS_TRACK_AND_NONNULL (audio_track));

  /* get master track */
  Track * master_track = P_MASTER_TRACK;

  test_recording_takes (ins_track, audio_track, master_track);

  test_helper_zrythm_cleanup ();
}

static void
test_automation_touch_recording (void)
{
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an instrument track from helm */
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);

  int     ins_track_pos = TRACKLIST->num_tracks - 1;
  Track * ins_track = TRACKLIST->tracks[ins_track_pos];

  prepare ();
  ins_track = TRACKLIST->tracks[ins_track_pos];

  prepare ();
  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT, true);

  /* enable loop & disable punch */
  transport_set_loop (TRANSPORT, true, true);
  position_set_to_bar (&TRANSPORT->loop_end_pos, 5);
  transport_set_punch_mode_enabled (TRANSPORT, false);

#define FRAMES_BEFORE_LOOP 4

  /* move playhead to 4 ticks before loop */
  Position pos;
  position_set_to_pos (&pos, &TRANSPORT->loop_end_pos);
  position_add_frames (
    &pos, -(CYCLE_SIZE + FRAMES_BEFORE_LOOP));
  transport_set_playhead_pos (TRANSPORT, &pos);

  /* enable recording for audio & ins track */
  Port *            touch_port;
  AutomationTrack * touch_at;

  /* enable recording for automation */
  g_assert_cmpint (
    ins_track->automation_tracklist.num_ats, >=, 3);
  touch_at = ins_track->automation_tracklist.ats[1];
  touch_at->record_mode = AUTOMATION_RECORD_MODE_TOUCH;
  automation_track_set_automation_mode (
    touch_at, AUTOMATION_MODE_RECORD, F_NO_PUBLISH_EVENTS);
  touch_port = port_find_from_identifier (&touch_at->port_id);
  g_assert_nonnull (touch_port);
  float touch_val_at_start = 0.1f;
  port_set_control_value (
    touch_port, touch_val_at_start, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  /* run the engine for 1 cycle */
  g_message ("--- processing engine...");
  SET_CACHES_AND_PROCESS;
  g_message ("--- processing recording events...");
  recording_manager_process_events (RECORDING_MANAGER);

  ZRegion *        touch_r;
  ArrangerObject * touch_r_obj;

  /* assert that automation events are created */
  g_assert_cmpint (touch_at->num_regions, ==, 1);

  /* alter automation */
  float touch_val_at_end = 0.2f;
  port_set_control_value (
    touch_port, touch_val_at_end, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  /* run the engine for 1 cycle (including loop) */
  g_message ("--- processing engine...");
  SET_CACHES_AND_PROCESS;
  g_message ("--- processing recording events...");
  recording_manager_process_events (RECORDING_MANAGER);

  g_assert_cmpint (touch_at->num_regions, ==, 2);
  touch_r = touch_at->regions[0];
  touch_r_obj = (ArrangerObject *) touch_r;
  position_set_to_pos (&pos, &TRANSPORT->loop_end_pos);
  g_assert_cmppos (&pos, &touch_r_obj->end_pos);
  position_add_frames (
    &pos, -(CYCLE_SIZE + FRAMES_BEFORE_LOOP));
  g_assert_cmppos (&pos, &touch_r_obj->pos);
  position_from_frames (
    &pos, (CYCLE_SIZE + FRAMES_BEFORE_LOOP));
  g_assert_cmppos (&pos, &touch_r_obj->loop_end_pos);
  touch_r = touch_at->regions[1];
  touch_r_obj = (ArrangerObject *) touch_r;
  position_set_to_pos (&pos, &TRANSPORT->loop_start_pos);
  g_assert_cmppos (&pos, &touch_r_obj->pos);
  position_add_frames (&pos, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &touch_r_obj->end_pos);
  position_from_frames (&pos, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  g_assert_cmppos (&pos, &touch_r_obj->loop_end_pos);

#undef FRAMES_BEFORE_LOOP

  /* assert that automation points are created */
  touch_r = touch_at->regions[0];
  touch_r_obj = (ArrangerObject *) touch_r;
  g_assert_cmpint (touch_r->num_aps, ==, 2);
  AutomationPoint * ap = touch_r->aps[0];
  ArrangerObject *  ap_obj = (ArrangerObject *) ap;
  position_init (&pos);
  g_assert_cmppos (&pos, &ap_obj->pos);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, touch_val_at_start, 0.0001f);
  ap = touch_r->aps[1];
  ap_obj = (ArrangerObject *) ap;
  position_init (&pos);
  position_add_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &ap_obj->pos);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, touch_val_at_end, 0.0001f);

  test_helper_zrythm_cleanup ();
}

static void
test_mono_recording (void)
{
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an audio track */
  Track * audio_track =
    track_create_empty_with_action (TRACK_TYPE_AUDIO, NULL);

  prepare ();
  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT, true);

  /* disable loop & punch */
  transport_set_loop (TRANSPORT, false, true);
  transport_set_punch_mode_enabled (TRANSPORT, false);

  /* move playhead to 2.1.1.0 */
  Position pos;
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  transport_set_playhead_pos (TRANSPORT, &pos);

  /* enable recording for audio track */
  track_set_recording (audio_track, true, false);

  /* set mono */
  TrackProcessor * audio_track_processor =
    audio_track->processor;
  port_set_control_value (
    audio_track_processor->mono, 1.f, true,
    F_NO_PUBLISH_EVENTS);

  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input->l->buf[i] = AUDIO_VAL;
      AUDIO_ENGINE->dummy_input->r->buf[i] = 0.f;
    }

  /* run the engine for 1 cycle */
  SET_CACHES_AND_PROCESS;
  recording_manager_process_events (RECORDING_MANAGER);

  ZRegion *        audio_r;
  ArrangerObject * audio_r_obj;

  /* assert that audio events are created */
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 1);
  audio_r = audio_track->lanes[0]->regions[0];
  audio_r_obj = (ArrangerObject *) audio_r;
  position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &audio_r_obj->end_pos);
  position_add_frames (&pos, -CYCLE_SIZE);
  g_assert_cmppos (&pos, &audio_r_obj->pos);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &audio_r_obj->loop_end_pos);

  /* assert that audio is correct */
  AudioClip * clip = audio_region_get_clip (audio_r);
  g_assert_cmpuint (clip->num_frames, ==, CYCLE_SIZE);
  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      g_assert_cmpfloat_with_epsilon (
        clip->ch_frames[0][i], AUDIO_VAL, 0.000001f);
      g_assert_cmpfloat_with_epsilon (
        clip->ch_frames[1][i], AUDIO_VAL, 0.000001f);
    }

  /* stop recording */
  track_set_recording (audio_track, false, false);

  /* run engine 1 more cycle to finalize recording */
  SET_CACHES_AND_PROCESS;
  transport_request_pause (TRANSPORT, true);
  recording_manager_process_events (RECORDING_MANAGER);

  /* save and undo/redo */
  test_project_save_and_reload ();
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_long_audio_recording (void)
{
  test_helper_zrythm_init ();

#ifdef TEST_WAV2

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an audio track */
  Track * audio_track =
    track_create_empty_with_action (TRACK_TYPE_AUDIO, NULL);

  prepare ();
  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT, true);

  /* disable loop & punch */
  transport_set_loop (TRANSPORT, false, true);
  transport_set_punch_mode_enabled (TRANSPORT, false);

  /* move playhead to 2.1.1.0 */
  Position pos, init_pos;
  position_set_to_bar (&pos, PLAYHEAD_START_BAR);
  transport_set_playhead_pos (TRANSPORT, &pos);
  position_set_to_pos (&init_pos, &pos);

  /* enable recording for audio track */
  track_set_recording (audio_track, true, false);

  AudioClip * clip = audio_clip_new_from_file (TEST_WAV2);
  unsigned_frame_t processed_ch_frames = 0;

  double total_secs_to_process =
    (double) clip->num_frames
    / (double) AUDIO_ENGINE->sample_rate;

  /* process almost whole clip */
  unsigned_frame_t total_samples_to_process =
    ((unsigned_frame_t) total_secs_to_process - 1)
    * AUDIO_ENGINE->sample_rate;

  unsigned_frame_t total_loops =
    total_samples_to_process / CYCLE_SIZE;

  ZRegion *        audio_r;
  ArrangerObject * audio_r_obj;

  /* run the engine for a few cycles */
  for (unsigned_frame_t j = 0; j < total_loops; j++)
    {
      for (nframes_t i = 0; i < CYCLE_SIZE; i++)
        {
          AUDIO_ENGINE->dummy_input->l->buf[i] =
            clip->ch_frames[0][processed_ch_frames];
          AUDIO_ENGINE->dummy_input->r->buf[i] =
            clip->ch_frames[1][processed_ch_frames];
          processed_ch_frames++;
        }

      SET_CACHES_AND_PROCESS;
      recording_manager_process_events (RECORDING_MANAGER);

      /* assert that audio events are created */
      g_assert_cmpint (
        audio_track->lanes[0]->num_regions, ==, 1);
      audio_r = audio_track->lanes[0]->regions[0];
      audio_r_obj = (ArrangerObject *) audio_r;
      position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
      g_assert_cmppos (&pos, &audio_r_obj->end_pos);
      position_add_frames (&pos, -CYCLE_SIZE);
      g_assert_cmppos (&init_pos, &audio_r_obj->pos);

      /* assert that audio is correct */
      AudioClip * r_clip = audio_region_get_clip (audio_r);
      g_assert_cmpuint (
        r_clip->num_frames, ==, processed_ch_frames);
      for (nframes_t i = 0; i < processed_ch_frames; i++)
        {
          g_assert_cmpfloat_with_epsilon (
            r_clip->ch_frames[0][i], clip->ch_frames[0][i],
            0.000001f);
          g_assert_cmpfloat_with_epsilon (
            r_clip->ch_frames[1][i], clip->ch_frames[1][i],
            0.000001f);
        }

      /* load the region file and check that
       * frames are correct */
      AudioClip * new_clip = audio_clip_new_from_file (
        audio_clip_get_path_in_pool (r_clip, F_NOT_BACKUP));
      if (r_clip->num_frames < new_clip->num_frames)
        {
          g_warning (
            "%zu < %zu", r_clip->num_frames,
            new_clip->num_frames);
        }
      g_warn_if_fail (audio_frames_equal (
        r_clip->ch_frames[0], new_clip->ch_frames[0],
        (size_t) MIN (new_clip->num_frames, r_clip->num_frames),
        0.0001f));
      g_warn_if_fail (audio_frames_equal (
        r_clip->frames, new_clip->frames,
        (size_t) MIN (
          new_clip->num_frames * 2, r_clip->num_frames * 2),
        0.0001f));
      audio_clip_free (new_clip);
    }

  /* stop recording */
  track_set_recording (audio_track, false, false);

  /* run engine 1 more cycle to finalize recording */
  SET_CACHES_AND_PROCESS;
  transport_request_pause (TRANSPORT, true);
  recording_manager_process_events (RECORDING_MANAGER);

  /* save and undo/redo */
  test_project_save_and_reload ();

  /* load the region file and check that
   * frames are correct */
  audio_track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 1);
  audio_r = audio_track->lanes[0]->regions[0];
  audio_r_obj = (ArrangerObject *) audio_r;
  AudioClip * r_clip = audio_region_get_clip (audio_r);
  AudioClip * new_clip = audio_clip_new_from_file (
    audio_clip_get_path_in_pool (r_clip, F_NOT_BACKUP));
  g_warn_if_fail (audio_frames_equal (
    r_clip->frames, new_clip->frames,
    (size_t) MIN (new_clip->num_frames, r_clip->num_frames),
    0.0001f));
  audio_clip_free (new_clip);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

#endif

  test_helper_zrythm_cleanup ();
}

/**
 * Test recording silent 2nd audio region does
 * not contain audio from other regions.
 */
static void
test_2nd_audio_recording (void)
{
  test_helper_zrythm_init ();

#ifdef TEST_WAV2

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an audio track from the file */
  SupportedFile * file =
    supported_file_new_from_path (TEST_WAV2);
  Track * audio_track = track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, NULL, TRACKLIST->num_tracks,
    1, NULL);

  prepare ();
  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT, true);

  /* disable loop & punch */
  transport_set_loop (TRANSPORT, false, true);
  transport_set_punch_mode_enabled (TRANSPORT, false);

  /* move playhead to 1.1.1.0 */
  Position pos, init_pos;
  position_set_to_bar (&pos, 1);
  transport_set_playhead_pos (TRANSPORT, &pos);
  position_set_to_pos (&init_pos, &pos);

  /* enable recording for audio track */
  track_set_recording (audio_track, true, false);

  AudioClip * clip = audio_clip_new_from_file (TEST_WAV2);
  unsigned_frame_t processed_ch_frames = 0;

  double total_secs_to_process =
    (double) clip->num_frames
    / (double) AUDIO_ENGINE->sample_rate;

  /* process almost whole clip */
  unsigned_frame_t total_samples_to_process =
    ((unsigned_frame_t) total_secs_to_process - 1)
    * AUDIO_ENGINE->sample_rate;

  unsigned_frame_t total_loops =
    total_samples_to_process / CYCLE_SIZE;

  ZRegion *        audio_r;
  ArrangerObject * audio_r_obj;

  /* run the engine for a few cycles */
  for (unsigned_frame_t j = 0; j < total_loops; j++)
    {
      /* clear audio input */
      for (nframes_t i = 0; i < CYCLE_SIZE; i++)
        {
          AUDIO_ENGINE->dummy_input->l->buf[i] = 0.f;
          AUDIO_ENGINE->dummy_input->r->buf[i] = 0.f;
          processed_ch_frames++;
        }

      SET_CACHES_AND_PROCESS;
      recording_manager_process_events (RECORDING_MANAGER);

      /* assert that audio events are created */
      g_assert_cmpint (
        audio_track->lanes[1]->num_regions, ==, 1);
      audio_r = audio_track->lanes[1]->regions[0];
      audio_r_obj = (ArrangerObject *) audio_r;
      position_set_to_pos (&pos, &TRANSPORT->playhead_pos);
      g_assert_cmppos (&pos, &audio_r_obj->end_pos);
      position_add_frames (&pos, -CYCLE_SIZE);
      g_assert_cmppos (&init_pos, &audio_r_obj->pos);

      /* assert that audio is silent */
      AudioClip * r_clip = audio_region_get_clip (audio_r);
      g_assert_cmpuint (
        r_clip->num_frames, ==, processed_ch_frames);
      for (nframes_t i = 0; i < processed_ch_frames; i++)
        {
          g_assert_cmpfloat_with_epsilon (
            r_clip->ch_frames[0][i], 0.f, 0.000001f);
          g_assert_cmpfloat_with_epsilon (
            r_clip->ch_frames[1][i], 0.f, 0.000001f);
        }
    }

  /* stop recording */
  track_set_recording (audio_track, false, false);

  /* run engine 1 more cycle to finalize recording */
  SET_CACHES_AND_PROCESS;
  transport_request_pause (TRANSPORT, true);
  recording_manager_process_events (RECORDING_MANAGER);

  /* save and undo/redo */
  test_project_save_and_reload ();

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

#endif

  test_helper_zrythm_cleanup ();
}

/**
 * Test chord track recording.
 */
static void
test_chord_track_recording (void)
{
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  prepare ();
  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT, true);

  /* set loop */
  transport_set_loop (TRANSPORT, true, true);

  /* move playhead to a few ticks before loop
   * end */
  Position pos, init_pos;
  position_set_to_pos (&pos, &TRANSPORT->loop_end_pos);
  position_add_frames (&pos, -CYCLE_SIZE);
  transport_set_playhead_pos (TRANSPORT, &pos);
  position_set_to_pos (&init_pos, &pos);

  /* enable recording for audio track */
  track_set_recording (
    P_CHORD_TRACK, true, F_NO_PUBLISH_EVENTS);

  /* run the engine for a few cycles */
  for (int i = 0; i < 4; i++)
    {
      SET_CACHES_AND_PROCESS;
      recording_manager_process_events (RECORDING_MANAGER);
    }

  /* assert that no events are created for now */
  g_assert_cmpint (P_CHORD_TRACK->num_chord_regions, ==, 1);

  /* stop recording */
  if (track_type_can_record (P_CHORD_TRACK->type))
    track_set_recording (
      P_CHORD_TRACK, false, F_NO_PUBLISH_EVENTS);

  /* run engine 1 more cycle to finalize
   * recording */
  SET_CACHES_AND_PROCESS;
  transport_request_pause (TRANSPORT, true);
  recording_manager_process_events (RECORDING_MANAGER);

  /* save and undo/redo */
  test_project_save_and_reload ();

  if (track_type_can_record (P_CHORD_TRACK->type))
    {
      undo_manager_undo (UNDO_MANAGER, NULL);
      undo_manager_redo (UNDO_MANAGER, NULL);
    }

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/integration/recording/"

  g_test_add_func (
    TEST_PREFIX "test_recording", (GTestFunc) test_recording);
  g_test_add_func (
    TEST_PREFIX "test automation touch recording",
    (GTestFunc) test_automation_touch_recording);
  g_test_add_func (
    TEST_PREFIX "test 2nd audio recording",
    (GTestFunc) test_2nd_audio_recording);
  g_test_add_func (
    TEST_PREFIX "test chord track recording",
    (GTestFunc) test_chord_track_recording);
  g_test_add_func (
    TEST_PREFIX "test long audio recording",
    (GTestFunc) test_long_audio_recording);
  g_test_add_func (
    TEST_PREFIX "test mono recording",
    (GTestFunc) test_mono_recording);

  return g_test_run ();
}
