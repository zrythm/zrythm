// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/dsp/engine_dummy.h"
#include "common/dsp/master_track.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/midi_track.h"
#include "common/dsp/recording_manager.h"
#include "common/utils/flags.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

constexpr auto CYCLE_SIZE = 100;
constexpr auto NOTE_PITCH = 56;
constexpr auto PLAYHEAD_START_BAR = 2;

/** Constant value to set to the float buffer. */
constexpr auto AUDIO_VAL = 0.42f;

/** Automation value to set. */
constexpr auto AUTOMATION_VAL = 0.23f;

static void
set_caches_and_process ()
{
  AUDIO_ENGINE->run_.store (false);
  TRACKLIST->set_caches (CacheType::PlaybackSnapshots);
  AUDIO_ENGINE->run_.store (true);
  AUDIO_ENGINE->process (CYCLE_SIZE);
}

static void
prepare (void)
{
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create dummy input for audio recording */
  AUDIO_ENGINE->dummy_input_ =
    std::make_unique<StereoPorts> (true, "Dummy input", "dummy_input");
  AUDIO_ENGINE->dummy_input_->set_owner (AUDIO_ENGINE.get ());
  AUDIO_ENGINE->dummy_input_->allocate_bufs ();

  /* sleep for a bit because Port's last_change interferes
   * with touch automation recording if it's too close to the
   * current time */
  g_usleep (1000000); // 1 sec
}

static void
do_takes_no_loop_no_punch (
  InstrumentTrack * ins_track,
  AudioTrack *      audio_track,
  MasterTrack *     master_track)
{
  TRANSPORT->recording_ = true;
  TRANSPORT->request_roll (true);

  /* disable loop & punch */
  TRANSPORT->set_loop (false, true);
  TRANSPORT->set_punch_mode_enabled (false);

  /* move playhead to 2.1.1.0 */
  TRANSPORT->set_playhead_to_bar (PLAYHEAD_START_BAR);

  /* enable recording for audio & ins track */
  ins_track->set_recording (true, false);
  audio_track->set_recording (true, false);

  ControlPort *    latch_port, *touch_port, *on_port;
  AutomationTrack *latch_at, *touch_at, *on_at;

  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input_->get_l ().buf_[i] = 0.f;
      AUDIO_ENGINE->dummy_input_->get_r ().buf_[i] = 0.f;
    }

  /* enable recording for master track automation */
  ASSERT_GE (master_track->automation_tracklist_->ats_.size (), 3);
  latch_at = master_track->automation_tracklist_->ats_[0].get ();
  latch_at->record_mode_ = AutomationRecordMode::Latch;
  latch_at->set_automation_mode (AutomationMode::Record, false);
  latch_port = Port::find_from_identifier<ControlPort> (latch_at->port_id_);
  ASSERT_NONNULL (latch_port);
  float latch_val_at_start = latch_port->control_;
  touch_at = master_track->automation_tracklist_->ats_[1].get ();
  touch_at->record_mode_ = AutomationRecordMode::Touch;
  touch_at->set_automation_mode (AutomationMode::Record, false);
  touch_port = Port::find_from_identifier<ControlPort> (touch_at->port_id_);
  ASSERT_NONNULL (touch_port);
  float touch_val_at_start = touch_port->control_;
  // note: was index 1 before (same as above)
  on_at = master_track->automation_tracklist_->ats_.at (2).get ();
  on_port = Port::find_from_identifier<ControlPort> (on_at->port_id_);
  ASSERT_NONNULL (on_port);
  float on_val_at_start = on_port->control_;

  set_caches_and_process ();
  RECORDING_MANAGER->process_events ();

  z_info ("process 1 ended");

  /* assert that MIDI events are created */
  ASSERT_SIZE_EQ (ins_track->lanes_[0]->regions_, 1);
  auto     mr = ins_track->lanes_[0]->regions_[0];
  Position pos;
  pos = TRANSPORT->playhead_pos_;
  ASSERT_POSITION_EQ (pos, mr->end_pos_);
  pos.add_frames (-CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, mr->pos_);
  pos.from_frames (CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, mr->loop_end_pos_);

  /* assert that audio events are created */
  ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 1);
  auto audio_r = audio_track->lanes_[0]->regions_[0];
  pos = TRANSPORT->playhead_pos_;
  ASSERT_POSITION_EQ (pos, audio_r->end_pos_);
  pos.add_frames (-CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, audio_r->pos_);
  pos.from_frames (CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, audio_r->loop_end_pos_);

  /* assert that audio is silent */
  AudioClip * clip = audio_r->get_clip ();
  ASSERT_EQ (clip->num_frames_, CYCLE_SIZE);
  for (int i = 0; i < 2; i++)
    {
      ASSERT_TRUE (
        audio_frames_empty (clip->ch_frames_.getReadPointer (i), CYCLE_SIZE));
    }

  /* assert that automation events are created */
  ASSERT_SIZE_EQ (latch_at->regions_, 1);
  auto latch_r = latch_at->regions_[0];
  pos = TRANSPORT->playhead_pos_;
  ASSERT_POSITION_EQ (pos, latch_r->end_pos_);
  pos.add_frames (-CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, latch_r->pos_);
  pos.from_frames (CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, latch_r->loop_end_pos_);
  ASSERT_EMPTY (touch_at->regions_);
  ASSERT_EMPTY (on_at->regions_);
  ASSERT_NEAR (latch_val_at_start, latch_port->control_, 0.0001f);
  ASSERT_NEAR (touch_val_at_start, touch_port->control_, 0.0001f);
  ASSERT_NEAR (on_val_at_start, on_port->control_, 0.0001f);

  /* assert that automation points are created */
  ASSERT_SIZE_EQ (latch_r->aps_, 1);
  auto ap = latch_r->aps_[0].get ();
  pos.zero ();
  ASSERT_POSITION_EQ (pos, ap->pos_);
  ASSERT_NEAR (ap->fvalue_, latch_val_at_start, 0.0001f);

  /* send a MIDI event */
  auto port = ins_track->processor_->midi_in_.get ();
  port->midi_events_.queued_events_.add_note_on (1, NOTE_PITCH, 70, 0);
  port->midi_events_.queued_events_.add_note_off (1, NOTE_PITCH, CYCLE_SIZE - 1);

  /* send audio data */
  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input_->get_l ().buf_[i] = AUDIO_VAL;
      AUDIO_ENGINE->dummy_input_->get_r ().buf_[i] = -AUDIO_VAL;
    }

  /* send automation data */
  latch_port->set_control_value (AUTOMATION_VAL, F_NOT_NORMALIZED, false);

  /* run the engine */
  set_caches_and_process ();
  RECORDING_MANAGER->process_events ();

  z_info ("process 2 ended");

  /* verify MIDI region positions */
  long r_length_frames = mr->get_length_in_frames ();
  pos.set_to_bar (PLAYHEAD_START_BAR);
  ASSERT_POSITION_EQ (pos, mr->pos_);
  pos.zero ();
  pos.add_frames (r_length_frames);
  ASSERT_POSITION_EQ (pos, mr->loop_end_pos_);
  pos = TRANSPORT->playhead_pos_;
  ASSERT_POSITION_EQ (pos, mr->end_pos_);

  /* verify that MIDI region contains the note at the
   * correct position */
  ASSERT_SIZE_EQ (mr->midi_notes_, 1);
  auto mn = mr->midi_notes_[0];
  ASSERT_EQ (mn->val_, NOTE_PITCH);
  long mn_length_frames = mn->get_length_in_frames ();
  ASSERT_EQ (mn_length_frames, CYCLE_SIZE);
  pos.from_frames (CYCLE_SIZE);
  ASSERT_POSITION_EQ (mn->pos_, pos);

  /* verify audio region positions */
  r_length_frames = audio_r->get_length_in_frames ();
  pos.set_to_bar (PLAYHEAD_START_BAR);
  ASSERT_POSITION_EQ (pos, audio_r->pos_);
  pos.zero ();
  pos.add_frames (r_length_frames);
  ASSERT_POSITION_EQ (pos, audio_r->loop_end_pos_);
  pos = TRANSPORT->playhead_pos_;
  ASSERT_POSITION_EQ (pos, audio_r->end_pos_);

  /* verify audio region contains the correct
   * audio data */
  clip = audio_r->get_clip ();
  ASSERT_EQ (clip->num_frames_, CYCLE_SIZE * 2);
  for (nframes_t i = CYCLE_SIZE; i < 2 * CYCLE_SIZE; i++)
    {
      ASSERT_NEAR (clip->ch_frames_.getSample (0, i), AUDIO_VAL, 0.000001f);
      ASSERT_NEAR (clip->ch_frames_.getSample (1, i), -AUDIO_VAL, 0.000001f);
    }

  /* verify automation region positions */
  r_length_frames = latch_r->get_length_in_frames ();
  pos.set_to_bar (PLAYHEAD_START_BAR);
  ASSERT_POSITION_EQ (pos, latch_r->pos_);
  pos.zero ();
  pos.add_frames (r_length_frames);
  ASSERT_POSITION_EQ (pos, latch_r->loop_end_pos_);
  pos = TRANSPORT->playhead_pos_;
  ASSERT_POSITION_EQ (pos, latch_r->end_pos_);

  /* verify automation data is added */
  ASSERT_SIZE_EQ (latch_r->aps_, 2);
  ap = latch_r->aps_[0].get ();
  pos.zero ();
  ASSERT_POSITION_EQ (pos, ap->pos_);
  ASSERT_NEAR (ap->fvalue_, latch_val_at_start, 0.0001f);
  ap = latch_r->aps_[1].get ();
  pos.zero ();
  pos.add_frames (CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, ap->pos_);
  ASSERT_NEAR (ap->fvalue_, AUTOMATION_VAL, 0.0001f);

  /* stop recording */
  ins_track->set_recording (false, false);
  audio_track->set_recording (false, false);
  latch_at->automation_mode_ = AutomationMode::Read;

  /* run engine 1 more cycle to finalize recording */
  set_caches_and_process ();
  TRANSPORT->request_pause (true);
  RECORDING_MANAGER->process_events ();

  ASSERT_EQ (RECORDING_MANAGER->num_active_recordings_, 0);

  z_info ("process 3 ended");

  /* save and undo/redo */
  test_project_save_and_reload ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();
}

static void
do_takes_loop_no_punch (
  InstrumentTrack * ins_track,
  AudioTrack *      audio_track,
  MasterTrack *     master_track)
{
  TRANSPORT->recording_ = true;
  TRANSPORT->request_roll (true);

  /* enable loop & disable punch */
  TRANSPORT->set_loop (true, true);
  TRANSPORT->loop_end_pos_.set_to_bar (5);
  TRANSPORT->set_punch_mode_enabled (false);

#define FRAMES_BEFORE_LOOP 4

  /* move playhead to 4 ticks before loop */
  Position pos;
  pos = TRANSPORT->loop_end_pos_;
  pos.add_frames (-FRAMES_BEFORE_LOOP);
  TRANSPORT->set_playhead_pos (pos);

  /* enable recording for audio & ins track */
  ins_track->set_recording (true, false);
  audio_track->set_recording (true, false);

  ControlPort *    latch_port, *touch_port, *on_port;
  AutomationTrack *latch_at, *touch_at, *on_at;

  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input_->get_l ().buf_[i] = 0.f;
      AUDIO_ENGINE->dummy_input_->get_r ().buf_[i] = 0.f;
    }

  /* enable recording for master track automation */
  ASSERT_GE (master_track->automation_tracklist_->ats_.size (), 3);
  latch_at = master_track->automation_tracklist_->ats_[0].get ();
  latch_at->record_mode_ = AutomationRecordMode::Latch;
  latch_at->set_automation_mode (AutomationMode::Record, false);
  latch_port = Port::find_from_identifier<ControlPort> (latch_at->port_id_);
  ASSERT_NONNULL (latch_port);
  float latch_val_at_start = latch_port->control_;
  touch_at = master_track->automation_tracklist_->ats_[1].get ();
  touch_at->record_mode_ = AutomationRecordMode::Touch;
  touch_at->set_automation_mode (AutomationMode::Record, false);
  touch_port = Port::find_from_identifier<ControlPort> (touch_at->port_id_);
  ASSERT_NONNULL (touch_port);
  float touch_val_at_start = touch_port->control_;
  // FIXME ???
  z_return_if_reached ();
  on_at = master_track->automation_tracklist_->ats_[1].get ();
  on_port = Port::find_from_identifier<ControlPort> (on_at->port_id_);
  ASSERT_NONNULL (on_port);
  float on_val_at_start = on_port->control_;

  /* run the engine for 1 cycle */
  z_info ("--- processing engine...");
  set_caches_and_process ();
  z_info ("--- processing recording events...");
  RECORDING_MANAGER->process_events ();

  /* assert that MIDI events are created */
  /* FIXME this assumes it runs after the first test */
  ASSERT_SIZE_EQ (ins_track->lanes_[1]->regions_, 1);
  ASSERT_SIZE_EQ (ins_track->lanes_[2]->regions_, 1);
  auto mr = ins_track->lanes_[1]->regions_[0].get ();
  pos = TRANSPORT->loop_end_pos_;
  ASSERT_POSITION_EQ (pos, mr->end_pos_);
  pos.add_frames (-FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, mr->pos_);
  pos.from_frames (FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, mr->loop_end_pos_);
  mr = ins_track->lanes_[2]->regions_[0].get ();
  pos = TRANSPORT->loop_start_pos_;
  ASSERT_POSITION_EQ (pos, mr->pos_);
  pos.add_frames (CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, mr->end_pos_);
  pos.from_frames (CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, mr->loop_end_pos_);

  /* assert that audio events are created */
  ASSERT_SIZE_EQ (audio_track->lanes_[1]->regions_, 1);
  ASSERT_SIZE_EQ (audio_track->lanes_[2]->regions_, 1);
  auto audio_r = audio_track->lanes_[1]->regions_[0].get ();
  pos = TRANSPORT->loop_end_pos_;
  ASSERT_POSITION_EQ (pos, audio_r->end_pos_);
  pos.add_frames (-FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, audio_r->pos_);
  pos.from_frames (FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, audio_r->loop_end_pos_);
  /* assert that audio is silent */
  AudioClip * clip = audio_r->get_clip ();
  ASSERT_EQ (clip->num_frames_, FRAMES_BEFORE_LOOP);
  for (nframes_t i = 0; i < (nframes_t) FRAMES_BEFORE_LOOP; i++)
    {
      ASSERT_NEAR (clip->ch_frames_.getSample (0, i), 0.f, 0.000001f);
      ASSERT_NEAR (clip->ch_frames_.getSample (1, i), 0.f, 0.000001f);
    }
  audio_r = audio_track->lanes_[2]->regions_[0].get ();
  pos = TRANSPORT->loop_start_pos_;
  ASSERT_POSITION_EQ (pos, audio_r->pos_);
  pos.add_frames (CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, audio_r->end_pos_);
  pos.from_frames (CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, audio_r->loop_end_pos_);
  /* assert that audio is silent */
  clip = audio_r->get_clip ();
  ASSERT_EQ (clip->num_frames_, CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  for (nframes_t i = 0; i < CYCLE_SIZE - (nframes_t) FRAMES_BEFORE_LOOP; i++)
    {
      ASSERT_NEAR (clip->ch_frames_.getSample (0, i), 0.f, 0.000001f);
      ASSERT_NEAR (clip->ch_frames_.getSample (1, i), 0.f, 0.000001f);
    }

  /* assert that automation events are created */
  ASSERT_SIZE_EQ (latch_at->regions_, 2);
  auto latch_r = latch_at->regions_[0].get ();
  pos = TRANSPORT->loop_end_pos_;
  ASSERT_POSITION_EQ (pos, latch_r->end_pos_);
  pos.add_frames (-FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, latch_r->pos_);
  pos.from_frames (FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, latch_r->loop_end_pos_);
  latch_r = latch_at->regions_[1].get ();
  pos = TRANSPORT->loop_start_pos_;
  ASSERT_POSITION_EQ (pos, latch_r->pos_);
  pos.add_frames (CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, latch_r->end_pos_);
  pos.from_frames (CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, latch_r->loop_end_pos_);

  ASSERT_EMPTY (touch_at->regions_);
  ASSERT_EMPTY (on_at->regions_);
  ASSERT_NEAR (latch_val_at_start, latch_port->control_, 0.0001f);
  ASSERT_NEAR (touch_val_at_start, touch_port->control_, 0.0001f);
  ASSERT_NEAR (on_val_at_start, on_port->control_, 0.0001f);

  /* assert that automation points are created */
  latch_r = latch_at->regions_[0].get ();
  ASSERT_SIZE_EQ (latch_r->aps_, 1);
  auto ap = latch_r->aps_[0].get ();
  pos.zero ();
  ASSERT_POSITION_EQ (pos, ap->pos_);
  ASSERT_NEAR (ap->fvalue_, latch_val_at_start, 0.0001f);
#if 0
  ap = latch_r->aps_[1];
  ap_obj = (ArrangerObject *) ap;
  position_from_frames (&pos, FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, ap_obj->pos);
  ASSERT_NEAR (
    ap->fvalue, latch_val_at_start, 0.0001f);
#endif

  /* TODO try the above steps again with data like
   * below */
#if 0
  /* send a MIDI event */
  Port * port = ins_track->processor->midi_in;
  midi_events_add_note_on (
    port->midi_events_, 1, NOTE_PITCH, 70, 0,
    F_QUEUED);
  midi_events_add_note_off (
    port->midi_events_, 1, NOTE_PITCH, CYCLE_SIZE - 1,
    F_QUEUED);

  /* send audio data */
  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input->get_l ().buf_[i] =
        AUDIO_VAL;
      AUDIO_ENGINE->dummy_input->get_r ().buf_[i] =
        - AUDIO_VAL;
    }

  /* send automation data */
  latch_port->set_control_value (AUTOMATION_VAL, F_NOT_NORMALIZED,
    false);

  /* run the engine */
  set_caches_and_process ();
  recording_manager_process_events (
    RECORDING_MANAGER);

  /* verify MIDI region positions */
  long r_length_frames =
    arranger_object_get_length_in_frames (mr_obj);
  pos.set_to_bar (PLAYHEAD_START_BAR);
  ASSERT_POSITION_EQ (pos, mr_obj->pos);
  pos.zero();
  pos.add_frames (r_length_frames);
  ASSERT_POSITION_EQ (pos, mr_obj->loop_end_pos_);
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  ASSERT_POSITION_EQ (pos, mr_obj->end_pos_);

  /* verify that MIDI region contains the note at the
   * correct position */
  ASSERT_EQ (mr->num_midi_notes, 1);
  MidiNote * mn = mr->midi_notes[0];
  ArrangerObject * mn_obj = (ArrangerObject *) mn;
  ASSERT_EQ (mn->val, NOTE_PITCH);
  long mn_length_frames =
    arranger_object_get_length_in_frames (mn_obj);
  g_assert_cmpint (
    mn_length_frames, ==, CYCLE_SIZE);
  position_from_frames (&pos, CYCLE_SIZE);
  ASSERT_POSITION_EQ (mn_obj->pos, pos);

  /* verify audio region positions */
  r_length_frames =
    arranger_object_get_length_in_frames (
      audio_r_obj);
  pos.set_to_bar (PLAYHEAD_START_BAR);
  ASSERT_POSITION_EQ (pos, audio_r_obj->pos);
  pos.zero();
  pos.add_frames (r_length_frames);
  ASSERT_POSITION_EQ (pos, audio_r_obj->loop_end_pos_);
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  ASSERT_POSITION_EQ (pos, audio_r_obj->end_pos_);

  /* verify audio region contains the correct
   * audio data */
  g_assert_cmpuint (
    audio_r->num_frames, ==, CYCLE_SIZE * 2);
  for (nframes_t i = CYCLE_SIZE;
       i < 2 * CYCLE_SIZE; i++)
    {
      ASSERT_NEAR (
        audio_r->ch_frames[0][i], AUDIO_VAL,
        0.000001f);
      ASSERT_NEAR (
        audio_r->ch_frames[1][i], - AUDIO_VAL,
        0.000001f);
    }

  /* verify automation region positions */
  r_length_frames =
    arranger_object_get_length_in_frames (
      latch_r_obj);
  pos.set_to_bar (PLAYHEAD_START_BAR);
  ASSERT_POSITION_EQ (pos, latch_r_obj->pos);
  pos.zero();
  pos.add_frames (r_length_frames);
  g_assert_cmppos (
    &pos, &latch_r_obj->loop_end_pos_);
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  ASSERT_POSITION_EQ (pos, latch_r_obj->end_pos_);

  /* verify automation data is added */
  ASSERT_EQ (latch_r->num_aps, 2);
  ap = latch_r->aps_[0];
  ap_obj = (ArrangerObject *) ap;
  pos.zero();
  ASSERT_POSITION_EQ (pos, ap_obj->pos);
  ASSERT_NEAR (
    ap->fvalue, latch_val_at_start, 0.0001f);
  ap = latch_r->aps_[1];
  ap_obj = (ArrangerObject *) ap;
  pos.zero();
  pos.add_frames (CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, ap_obj->pos);
  ASSERT_NEAR (
    ap->fvalue, AUTOMATION_VAL, 0.0001f);

  /* stop recording */
  track_set_recording (ins_track, false, false);
  track_set_recording (audio_track, false, false);
  latch_at->automation_mode = AutomationMode::AUTOMATION_MODE_READ;

  /* run engine 1 more cycle to finalize recording */
  set_caches_and_process ();
  recording_manager_process_events (
    RECORDING_MANAGER);
#endif

#undef FRAMES_BEFORE_LOOP

  /* save and undo/redo */
  test_project_save_and_reload ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();
}

static void
test_recording_takes (
  InstrumentTrack * ins_track,
  AudioTrack *      audio_track,
  MasterTrack *     master_track)
{
  const int ins_track_pos = ins_track->pos_;
  const int audio_track_pos = audio_track->pos_;
  const int master_track_pos = master_track->pos_;
  ASSERT_LT (audio_track_pos, TRACKLIST->get_num_tracks ());

  prepare ();
  do_takes_no_loop_no_punch (ins_track, audio_track, master_track);

  ASSERT_LT (audio_track_pos, TRACKLIST->get_num_tracks ());
  ins_track = TRACKLIST->get_track<InstrumentTrack> (ins_track_pos);
  audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
  master_track = TRACKLIST->get_track<MasterTrack> (master_track_pos);

  prepare ();
  do_takes_loop_no_punch (ins_track, audio_track, master_track);
}

TEST_F (ZrythmFixture, Recording)
{
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an instrument track for testing */
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);
  auto ins_track = TRACKLIST->get_last_track<InstrumentTrack> ();

  /* create an audio track */
  auto audio_track = Track::create_empty_with_action<AudioTrack> ();
  ASSERT_TRUE (IS_TRACK_AND_NONNULL (audio_track));

  /* get master track */
  auto master_track = P_MASTER_TRACK;

  test_recording_takes (ins_track, audio_track, master_track);
}

TEST_F (ZrythmFixture, AutomationTouchRecording)
{
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an instrument track from helm */
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);

  int  ins_track_pos = TRACKLIST->get_last_pos ();
  auto ins_track = TRACKLIST->get_track<InstrumentTrack> (ins_track_pos);

  prepare ();
  ins_track = TRACKLIST->get_track<InstrumentTrack> (ins_track_pos);

  prepare ();
  TRANSPORT->recording_ = true;
  TRANSPORT->request_roll (true);

  /* enable loop & disable punch */
  TRANSPORT->set_loop (true, true);
  TRANSPORT->loop_end_pos_.set_to_bar (5);
  TRANSPORT->set_punch_mode_enabled (false);

#define FRAMES_BEFORE_LOOP 4

  /* move playhead to 4 ticks before loop */
  Position pos;
  pos = TRANSPORT->loop_end_pos_;
  pos.add_frames (-(CYCLE_SIZE + FRAMES_BEFORE_LOOP));
  TRANSPORT->set_playhead_pos (pos);

  /* enable recording for audio & ins track */
  ControlPort *     touch_port;
  AutomationTrack * touch_at;

  /* enable recording for automation */
  ASSERT_GE (ins_track->automation_tracklist_->ats_.size (), 3);
  touch_at = ins_track->automation_tracklist_->ats_[1].get ();
  touch_at->record_mode_ = AutomationRecordMode::Touch;
  touch_at->set_automation_mode (AutomationMode::Record, false);
  touch_port = Port::find_from_identifier<ControlPort> (touch_at->port_id_);
  ASSERT_NONNULL (touch_port);
  float touch_val_at_start = 0.1f;
  touch_port->set_control_value (touch_val_at_start, F_NOT_NORMALIZED, false);

  /* run the engine for 1 cycle */
  z_info ("--- processing engine...");
  set_caches_and_process ();
  z_info ("--- processing recording events...");
  RECORDING_MANAGER->process_events ();

  AutomationRegion * touch_r;

  /* assert that automation events are created */
  ASSERT_SIZE_EQ (touch_at->regions_, 1);

  /* alter automation */
  float touch_val_at_end = 0.2f;
  touch_port->set_control_value (touch_val_at_end, F_NOT_NORMALIZED, false);

  /* run the engine for 1 cycle (including loop) */
  z_info ("--- processing engine...");
  set_caches_and_process ();
  z_info ("--- processing recording events...");
  RECORDING_MANAGER->process_events ();

  ASSERT_SIZE_EQ (touch_at->regions_, 2);
  touch_r = touch_at->regions_[0].get ();
  pos = TRANSPORT->loop_end_pos_;
  ASSERT_POSITION_EQ (pos, touch_r->end_pos_);
  pos.add_frames (-(CYCLE_SIZE + FRAMES_BEFORE_LOOP));
  ASSERT_POSITION_EQ (pos, touch_r->pos_);
  pos.from_frames ((CYCLE_SIZE + FRAMES_BEFORE_LOOP));
  ASSERT_POSITION_EQ (pos, touch_r->loop_end_pos_);
  touch_r = touch_at->regions_[1].get ();
  pos = TRANSPORT->loop_start_pos_;
  ASSERT_POSITION_EQ (pos, touch_r->pos_);
  pos.add_frames (CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, touch_r->end_pos_);
  pos.from_frames (CYCLE_SIZE - FRAMES_BEFORE_LOOP);
  ASSERT_POSITION_EQ (pos, touch_r->loop_end_pos_);

#undef FRAMES_BEFORE_LOOP

  /* assert that automation points are created */
  touch_r = touch_at->regions_[0].get ();
  ASSERT_SIZE_EQ (touch_r->aps_, 2);
  auto ap = touch_r->aps_[0].get ();
  pos.zero ();
  ASSERT_POSITION_EQ (pos, ap->pos_);
  ASSERT_NEAR (ap->fvalue_, touch_val_at_start, 0.0001f);
  ap = touch_r->aps_[1].get ();
  pos.zero ();
  pos.add_frames (CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, ap->pos_);
  ASSERT_NEAR (ap->fvalue_, touch_val_at_end, 0.0001f);
}

TEST_F (ZrythmFixture, MonoRecording)
{
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an audio track */
  auto audio_track = Track::create_empty_with_action<AudioTrack> ();

  prepare ();
  TRANSPORT->recording_ = true;
  TRANSPORT->request_roll (true);

  /* disable loop & punch */
  TRANSPORT->set_loop (false, true);
  TRANSPORT->set_punch_mode_enabled (false);

  /* move playhead to 2.1.1.0 */
  Position pos;
  pos.set_to_bar (PLAYHEAD_START_BAR);
  TRANSPORT->set_playhead_pos (pos);

  /* enable recording for audio track */
  audio_track->set_recording (true, false);

  /* set mono */
  auto &audio_track_processor = audio_track->processor_;
  audio_track_processor->mono_->set_control_value (1.f, true, false);

  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      AUDIO_ENGINE->dummy_input_->get_l ().buf_[i] = AUDIO_VAL;
      AUDIO_ENGINE->dummy_input_->get_r ().buf_[i] = 0.f;
    }

  /* run the engine for 1 cycle */
  set_caches_and_process ();
  RECORDING_MANAGER->process_events ();

  AudioRegion * audio_r;

  /* assert that audio events are created */
  ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 1);
  audio_r = audio_track->lanes_[0]->regions_[0].get ();
  pos = TRANSPORT->playhead_pos_;
  ASSERT_POSITION_EQ (pos, audio_r->end_pos_);
  pos.add_frames (-CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, audio_r->pos_);
  pos.from_frames (CYCLE_SIZE);
  ASSERT_POSITION_EQ (pos, audio_r->loop_end_pos_);

  /* assert that audio is correct */
  auto clip = audio_r->get_clip ();
  ASSERT_EQ (clip->num_frames_, CYCLE_SIZE);
  for (nframes_t i = 0; i < CYCLE_SIZE; i++)
    {
      ASSERT_NEAR (clip->ch_frames_.getSample (0, i), AUDIO_VAL, 0.000001f);
      ASSERT_NEAR (clip->ch_frames_.getSample (1, i), AUDIO_VAL, 0.000001f);
    }

  /* stop recording */
  audio_track->set_recording (false, false);

  /* run engine 1 more cycle to finalize recording */
  set_caches_and_process ();
  TRANSPORT->request_pause (true);
  RECORDING_MANAGER->process_events ();

  /* save and undo/redo */
  test_project_save_and_reload ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
}

TEST_F (ZrythmFixture, LongAudioRecording)
{
#ifdef TEST_WAV2

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an audio track */
  auto audio_track = Track::create_empty_with_action<AudioTrack> ();

  prepare ();
  TRANSPORT->recording_ = true;
  TRANSPORT->request_roll (true);

  /* disable loop & punch */
  TRANSPORT->set_loop (false, true);
  TRANSPORT->set_punch_mode_enabled (false);

  /* move playhead to 2.1.1.0 */
  Position pos, init_pos;
  pos.set_to_bar (PLAYHEAD_START_BAR);
  TRANSPORT->set_playhead_pos (pos);
  init_pos = pos;

  /* enable recording for audio track */
  audio_track->set_recording (true, false);

  AudioClip        clip (TEST_WAV2);
  unsigned_frame_t processed_ch_frames = 0;

  double total_secs_to_process =
    (double) clip.num_frames_ / (double) AUDIO_ENGINE->sample_rate_;

  /* process almost whole clip */
  unsigned_frame_t total_samples_to_process =
    ((unsigned_frame_t) total_secs_to_process - 1) * AUDIO_ENGINE->sample_rate_;

  unsigned_frame_t total_loops = total_samples_to_process / CYCLE_SIZE;

  AudioRegion * audio_r;

  /* run the engine for a few cycles */
  for (unsigned_frame_t j = 0; j < total_loops; j++)
    {
      for (nframes_t i = 0; i < CYCLE_SIZE; i++)
        {
          AUDIO_ENGINE->dummy_input_->get_l ().buf_[i] =
            clip.ch_frames_.getSample (0, processed_ch_frames);
          AUDIO_ENGINE->dummy_input_->get_r ().buf_[i] =
            clip.ch_frames_.getSample (1, processed_ch_frames);
          processed_ch_frames++;
        }

      set_caches_and_process ();
      RECORDING_MANAGER->process_events ();

      /* assert that audio events are created */
      ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 1);
      audio_r = audio_track->lanes_[0]->regions_[0].get ();
      pos = TRANSPORT->playhead_pos_;
      ASSERT_POSITION_EQ (pos, audio_r->end_pos_);
      pos.add_frames (-CYCLE_SIZE);
      ASSERT_POSITION_EQ (init_pos, audio_r->pos_);

      /* assert that audio is correct */
      AudioClip * r_clip = audio_r->get_clip ();
      ASSERT_EQ (r_clip->num_frames_, processed_ch_frames);
      for (nframes_t i = 0; i < processed_ch_frames; i++)
        {
          ASSERT_NEAR (
            r_clip->ch_frames_.getSample (0, i),
            clip.ch_frames_.getSample (0, i), 0.000001f);
          ASSERT_NEAR (
            r_clip->ch_frames_.getSample (1, i),
            clip.ch_frames_.getSample (1, i), 0.000001f);
        }

      /* load the region file and check that frames are correct */
      AudioClip new_clip (r_clip->get_path_in_pool (false));
      if (r_clip->num_frames_ < new_clip.num_frames_)
        {
          z_error ("{} < {}", r_clip->num_frames_, new_clip.num_frames_);
        }
      z_return_if_fail (audio_frames_equal (
        r_clip->ch_frames_.getReadPointer (0),
        new_clip.ch_frames_.getReadPointer (0),
        (size_t) std::min (new_clip.num_frames_, r_clip->num_frames_), 0.0001f));
      z_return_if_fail (audio_frames_equal (
        r_clip->frames_.getReadPointer (0), new_clip.frames_.getReadPointer (0),
        (size_t) std::min (new_clip.num_frames_ * 2, r_clip->num_frames_ * 2),
        0.0001f));
    }

  /* stop recording */
  audio_track->set_recording (false, false);

  /* run engine 1 more cycle to finalize recording */
  set_caches_and_process ();
  TRANSPORT->request_pause (true);
  RECORDING_MANAGER->process_events ();

  /* save and undo/redo */
  test_project_save_and_reload ();

  /* load the region file and check that
   * frames are correct */
  audio_track = TRACKLIST->get_last_track<AudioTrack> ();
  ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 1);
  audio_r = audio_track->lanes_[0]->regions_[0].get ();
  AudioClip * r_clip = audio_r->get_clip ();
  AudioClip   new_clip (r_clip->get_path_in_pool (false));
  z_warn_if_fail (audio_frames_equal (
    r_clip->frames_.getReadPointer (0), new_clip.frames_.getReadPointer (0),
    (size_t) std::min (new_clip.num_frames_, r_clip->num_frames_), 0.0001f));

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

#endif
}

/* Test recording silent 2nd audio region does not contain audio from other
regions. */
TEST_F (ZrythmFixture, SecondAudioRecording)
{
#ifdef TEST_WAV2

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an audio track from the file */
  FileDescriptor file = FileDescriptor (TEST_WAV2);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, nullptr, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto audio_track = TRACKLIST->get_last_track<AudioTrack> ();

  prepare ();
  TRANSPORT->recording_ = true;
  TRANSPORT->request_roll (true);

  /* disable loop & punch */
  TRANSPORT->set_loop (false, true);
  TRANSPORT->set_punch_mode_enabled (false);

  /* move playhead to 1.1.1.0 */
  Position pos, init_pos;
  pos.set_to_bar (1);
  TRANSPORT->set_playhead_pos (pos);
  init_pos = pos;

  /* enable recording for audio track */
  audio_track->set_recording (true, false);

  AudioClip        clip (TEST_WAV2);
  unsigned_frame_t processed_ch_frames = 0;

  double total_secs_to_process =
    (double) clip.num_frames_ / (double) AUDIO_ENGINE->sample_rate_;

  /* process almost whole clip */
  unsigned_frame_t total_samples_to_process =
    ((unsigned_frame_t) total_secs_to_process - 1) * AUDIO_ENGINE->sample_rate_;

  unsigned_frame_t total_loops = total_samples_to_process / CYCLE_SIZE;

  /* run the engine for a few cycles */
  for (unsigned_frame_t j = 0; j < total_loops; j++)
    {
      /* clear audio input */
      for (nframes_t i = 0; i < CYCLE_SIZE; i++)
        {
          AUDIO_ENGINE->dummy_input_->get_l ().buf_[i] = 0.f;
          AUDIO_ENGINE->dummy_input_->get_r ().buf_[i] = 0.f;
          processed_ch_frames++;
        }

      set_caches_and_process ();
      RECORDING_MANAGER->process_events ();

      /* assert that audio events are created */
      ASSERT_SIZE_EQ (audio_track->lanes_[1]->regions_, 1);
      auto audio_r = audio_track->lanes_[1]->regions_[0].get ();
      pos = TRANSPORT->playhead_pos_;
      ASSERT_POSITION_EQ (pos, audio_r->end_pos_);
      pos.add_frames (-CYCLE_SIZE);
      ASSERT_POSITION_EQ (init_pos, audio_r->pos_);

      /* assert that audio is silent */
      AudioClip * r_clip = audio_r->get_clip ();
      ASSERT_EQ (r_clip->num_frames_, processed_ch_frames);
      for (nframes_t i = 0; i < processed_ch_frames; i++)
        {
          ASSERT_NEAR (r_clip->ch_frames_.getSample (0, i), 0.f, 0.000001f);
          ASSERT_NEAR (r_clip->ch_frames_.getSample (1, i), 0.f, 0.000001f);
        }
    }

  /* stop recording */
  audio_track->set_recording (false, false);

  /* run engine 1 more cycle to finalize recording */
  set_caches_and_process ();
  TRANSPORT->request_pause (true);
  RECORDING_MANAGER->process_events ();

  /* save and undo/redo */
  test_project_save_and_reload ();

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

#endif
}

TEST_F (ZrythmFixture, ChordTrackRecording)
{
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  prepare ();
  TRANSPORT->recording_ = true;
  TRANSPORT->request_roll (true);

  /* set loop */
  TRANSPORT->set_loop (true, true);

  /* move playhead to a few ticks before loop
   * end */
  Position pos, init_pos;
  pos = TRANSPORT->loop_end_pos_;
  pos.add_frames (-CYCLE_SIZE);
  TRANSPORT->set_playhead_pos (pos);
  init_pos = pos;

  /* enable recording for audio track */
  P_CHORD_TRACK->set_recording (true, false);

  /* run the engine for a few cycles */
  for (int i = 0; i < 4; i++)
    {
      set_caches_and_process ();
      RECORDING_MANAGER->process_events ();
    }

  /* assert that no events are created for now */
  ASSERT_SIZE_EQ (P_CHORD_TRACK->regions_, 1);

  /* stop recording */
  if (P_CHORD_TRACK->can_record ())
    P_CHORD_TRACK->set_recording (false, false);

  /* run engine 1 more cycle to finalize
   * recording */
  set_caches_and_process ();
  TRANSPORT->request_pause (true);
  RECORDING_MANAGER->process_events ();

  /* save and undo/redo */
  test_project_save_and_reload ();

  if (P_CHORD_TRACK->can_record ())
    {
      UNDO_MANAGER->undo ();
      UNDO_MANAGER->redo ();
    }
}