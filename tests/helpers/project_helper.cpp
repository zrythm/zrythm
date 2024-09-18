// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "tests/helpers/project_helper.h"

void
BootstrapTimelineFixture::SetUp ()
{
  ZrythmFixture::SetUp ();

  /* pause engine */
  AudioEngine::State state;
  AUDIO_ENGINE->wait_for_pause (state, false, true);

  /* remove any previous work */
  P_CHORD_TRACK->clear_objects ();
  P_MARKER_TRACK->clear_objects ();
  P_TEMPO_TRACK->clear_objects ();
  // modulator_track_clear (P_MODULATOR_TRACK);
  for (auto &track : TRACKLIST->tracks_ | std::views::reverse)
    {
      if (track->is_deletable ())
        {
          TRACKLIST->remove_track (*track, true, true, false, false);
        }
    }
  P_MASTER_TRACK->clear_objects ();

  /* Create and add a MidiRegion with a MidiNote */
  p1_.set_to_bar (2);
  p2_.set_to_bar (4);
  {
    auto midi_track = TRACKLIST->append_track (
      *MidiTrack::create_unique (MIDI_TRACK_NAME, TRACKLIST->tracks_.size ()),
      false, false);
    ASSERT_NONEMPTY (midi_track->name_);
    const auto midi_track_name_hash = midi_track->get_name_hash ();
    ASSERT_NE (midi_track_name_hash, 0);
    auto mr = std::make_shared<MidiRegion> (
      p1_, p2_, midi_track_name_hash, MIDI_REGION_LANE, 0);
    ASSERT_NO_THROW (
      midi_track->add_region (mr, nullptr, MIDI_REGION_LANE, true, false));
    mr->set_name (MIDI_REGION_NAME, false);
    ASSERT_EQ (mr->id_.track_name_hash_, midi_track_name_hash);
    ASSERT_EQ (mr->id_.lane_pos_, MIDI_REGION_LANE);
    ASSERT_EQ (mr->id_.type_, RegionType::Midi);
    TL_SELECTIONS->add_object_ref (mr);

    /* add a midi note to the region */
    auto mn = std::make_shared<MidiNote> (mr->id_, p1_, p2_, MN_VAL, MN_VEL);
    mr->append_object (mn);
    ASSERT_EQ (mn->region_id_, mr->id_);
    MIDI_SELECTIONS->add_object_ref (mn);
  }

  {
    /* Create and add an automation region with 2 AutomationPoint's */
    auto at = P_MASTER_TRACK->channel_->get_automation_track (
      PortIdentifier::Flags::StereoBalance);
    const auto master_track_name_hash = P_MASTER_TRACK->get_name_hash ();
    auto       automation_r = std::make_shared<AutomationRegion> (
      p1_, p2_, master_track_name_hash, at->index_, 0);
    ASSERT_NO_THROW (
      P_MASTER_TRACK->add_region (automation_r, at, 0, true, false));
    ASSERT_EQ (automation_r->id_.track_name_hash_, master_track_name_hash);
    ASSERT_EQ (automation_r->id_.at_idx_, at->index_);
    ASSERT_EQ (automation_r->id_.type_, RegionType::Automation);
    TL_SELECTIONS->add_object_ref (automation_r);

    /* add 2 automation points to the region */
    auto ap = std::make_shared<AutomationPoint> (AP_VAL1, AP_VAL1, p1_);
    automation_r->append_object (ap);
    AUTOMATION_SELECTIONS->add_object_ref (ap);
    ap = std::make_shared<AutomationPoint> (AP_VAL2, AP_VAL1, p2_);
    automation_r->append_object (ap);
    AUTOMATION_SELECTIONS->add_object_ref (ap);
  }

  {
    /* Create and add a chord region with 2 Chord's */
    auto chord_r = std::make_shared<ChordRegion> (p1_, p2_, 0);
    ASSERT_NO_THROW (
      P_CHORD_TRACK->Track::add_region (chord_r, nullptr, 0, true, false));
    TL_SELECTIONS->add_object_ref (chord_r);

    /* add 2 chords to the region */
    auto co = std::make_shared<ChordObject> (chord_r->id_, 0, 1);
    chord_r->append_object (co);
    co->pos_setter (&p1_);
    CHORD_SELECTIONS->add_object_ref (co);
    co = std::make_shared<ChordObject> (chord_r->id_, 0, 1);
    chord_r->append_object (co);
    co->pos_setter (&p2_);
    CHORD_SELECTIONS->add_object_ref (co);
  }

  {
    /* create and add a Marker */
    auto m = std::make_shared<Marker> (MARKER_NAME);
    P_MARKER_TRACK->add_marker (m);
    ASSERT_EQ (m->marker_track_index_, 2);
    TL_SELECTIONS->add_object_ref (m);
    m->pos_setter (&p1_);
  }

  {
    /* create and add a ScaleObject */
    MusicalScale ms (MUSICAL_SCALE_TYPE, MUSICAL_SCALE_ROOT);
    auto         s = std::make_shared<ScaleObject> (ms);
    P_CHORD_TRACK->add_scale (s);
    TL_SELECTIONS->add_object_ref (s);
    s->pos_setter (&p1_);
  }

  {
    /* Create and add an audio region */
    p1_.set_to_bar (2);
    auto audio_track = TRACKLIST->append_track (
      *AudioTrack::create_unique (
        AUDIO_TRACK_NAME, TRACKLIST->tracks_.size (), AUDIO_ENGINE->sample_rate_),
      false, false);
    ASSERT_SIZE_EQ (P_MASTER_TRACK->children_, 1);
    auto       audio_file_path = fs::path (TESTS_SRCDIR) / "test.wav";
    const auto track_name_hash = audio_track->get_name_hash ();
    std::shared_ptr<AudioRegion> r;
    ASSERT_NO_THROW (
      r = std::make_shared<AudioRegion> (
        -1, audio_file_path.string (), true, nullptr, 0, std::nullopt, 0,
        (BitDepth) 0, p1_, track_name_hash, AUDIO_REGION_LANE, 0));
    auto clip = r->get_clip ();
    ASSERT_GT (clip->num_frames_, 151000);
    ASSERT_LT (clip->num_frames_, 152000);
    ASSERT_NO_THROW (
      audio_track->add_region (r, nullptr, AUDIO_REGION_LANE, true, false));
    r->set_name (AUDIO_REGION_NAME, false);
    ASSERT_EQ (r->id_.track_name_hash_, track_name_hash);
    ASSERT_EQ (r->id_.lane_pos_, AUDIO_REGION_LANE);
    ASSERT_EQ (r->id_.type_, RegionType::Audio);
    TL_SELECTIONS->add_object_ref (r);
  }

  /* create the target tracks */
  TRACKLIST->append_track (
    *MidiTrack::create_unique (
      TARGET_MIDI_TRACK_NAME, TRACKLIST->tracks_.size ()),
    false, false);
  TRACKLIST->append_track (
    *AudioTrack::create_unique (
      TARGET_AUDIO_TRACK_NAME, TRACKLIST->tracks_.size (),
      AUDIO_ENGINE->sample_rate_),
    false, false);
  ASSERT_SIZE_EQ (P_MASTER_TRACK->children_, 2);

  AUDIO_ENGINE->update_frames_per_tick (
    P_TEMPO_TRACK->get_beats_per_bar (), P_TEMPO_TRACK->get_current_bpm (),
    AUDIO_ENGINE->sample_rate_, true, true, false);

  ROUTER->recalc_graph (false);

  AUDIO_ENGINE->resume (state);

  ASSERT_SIZE_EQ (TL_SELECTIONS->objects_, 6);
  test_project_save_and_reload ();
  ASSERT_SIZE_EQ (TL_SELECTIONS->objects_, 6);
  ASSERT_SIZE_EQ (P_MASTER_TRACK->children_, 2);
}

void
BootstrapTimelineFixture::check_vs_original_state (bool check_selections)
{
  bool after_reload = false;

check_vs_orig_state:

  if (check_selections)
    {
      ASSERT_EQ (TL_SELECTIONS->get_num_objects<Region> (), 4);
      ASSERT_EQ (TL_SELECTIONS->get_num_objects<Marker> (), 1);
      ASSERT_EQ (TL_SELECTIONS->get_num_objects<ScaleObject> (), 1);
    }

  auto midi_track = TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
  ASSERT_NONNULL (midi_track);
  auto audio_track =
    TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
  ASSERT_NONNULL (audio_track);

  Position p1_before_move = p1_;
  Position p2_before_move = p2_;
  p1_before_move.add_ticks (-MOVE_TICKS);
  p2_before_move.add_ticks (-MOVE_TICKS);

  { /* check midi region */
    ASSERT_SIZE_EQ (midi_track->lanes_, MIDI_REGION_LANE + 2);
    ASSERT_SIZE_EQ (midi_track->lanes_.at (MIDI_REGION_LANE)->regions_, 1);
    const auto &r = midi_track->lanes_.at (MIDI_REGION_LANE)->regions_.front ();
    ASSERT_POSITION_EQ (r->pos_, p1_);
    ASSERT_POSITION_EQ (r->end_pos_, p2_);
    ASSERT_SIZE_EQ (r->midi_notes_, 1);
    const auto &mn = r->midi_notes_.front ();
    ASSERT_EQ (mn->val_, MN_VAL);
    ASSERT_EQ (mn->vel_->vel_, MN_VEL);
    ASSERT_POSITION_EQ (mn->pos_, p1_);
    ASSERT_POSITION_EQ (mn->end_pos_, p2_);
    ASSERT_EQ (mn->region_id_, r->id_);
  }

  {
    /* check audio region */
    ASSERT_SIZE_EQ (audio_track->lanes_[AUDIO_REGION_LANE]->regions_, 1);
    auto r = audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0];
    ASSERT_POSITION_EQ (r->pos_, p1_);
  }

  {
    /* check automation region */
    const auto * at = P_MASTER_TRACK->channel_->get_automation_track (
      PortIdentifier::Flags::StereoBalance);
    ASSERT_NONNULL (at);
    ASSERT_SIZE_EQ (at->regions_, 1);
    const auto &r = at->regions_.at (0);
    ASSERT_POSITION_EQ (r->pos_, p1_);
    ASSERT_POSITION_EQ (r->end_pos_, p2_);
    ASSERT_SIZE_EQ (r->aps_, 2);
    const auto &ap1 = r->aps_.at (0);
    ASSERT_POSITION_EQ (ap1->pos_, p1_);
    ASSERT_NEAR (ap1->fvalue_, AP_VAL1, 0.000001f);
    const auto &ap2 = r->aps_[1];
    ASSERT_POSITION_EQ (ap2->pos_, p2_);
    ASSERT_NEAR (ap2->fvalue_, AP_VAL2, 0.000001f);
  }

  {
    /* check marker */
    ASSERT_SIZE_EQ (P_MARKER_TRACK->markers_, 3);
    {
      const auto &m = P_MARKER_TRACK->markers_.at (0);
      ASSERT_NONNULL (m);
      ASSERT_EQ (m->name_, "[start]");
    }
    {
      const auto &m = P_MARKER_TRACK->markers_.at (2);
      ASSERT_POSITION_EQ (m->pos_, p1_);
      ASSERT_EQ (m->name_, MARKER_NAME);
    }
  }

  {
    /* check scale object */
    ASSERT_SIZE_EQ (P_CHORD_TRACK->scales_, 1);
    auto s = P_CHORD_TRACK->scales_.at (0);
    ASSERT_POSITION_EQ (s->pos_, p1_);
    ASSERT_EQ (s->scale_.type_, MUSICAL_SCALE_TYPE);
    ASSERT_EQ (s->scale_.root_key_, MUSICAL_SCALE_ROOT);
  }

  /* save the project and reopen it. some callers undo after this step so this
   * checks if the undo history works after reopening the project */
  if (!after_reload)
    {
      test_project_save_and_reload ();
      after_reload = true;
      goto check_vs_orig_state;
    }
}

void
BootstrapTimelineFixture::TearDown ()
{

  ZrythmFixture::TearDown ();
}

fs::path
test_project_save ()
{
  /* save the project */
  EXPECT_NO_THROW (PROJECT->save (PROJECT->dir_, 0, 0, false));
  auto prj_file = fs::path (PROJECT->dir_) / PROJECT_FILE;

  AUDIO_ENGINE->activate (false);
  PROJECT.reset ();

  return prj_file;
}

void
test_project_reload (const fs::path &prj_file)
{
  std::make_unique<ProjectInitFlowManager> (
    prj_file.string (), false, test_helper_project_init_done_cb, nullptr);
}

void
test_project_save_and_reload (void)
{
  /* save the project */
  auto prj_file = test_project_save ();
  ASSERT_NONEMPTY (prj_file);

  /* recreate the recording manager to drop any events */
  gZrythm->recording_manager_ = std::make_unique<RecordingManager> ();

  /* reload it */
  test_project_reload (prj_file);
}

void
test_project_stop_dummy_engine ()
{
  AUDIO_ENGINE->dummy_audio_thread_->signalThreadShouldExit ();
  AUDIO_ENGINE->dummy_audio_thread_->waitForThreadToExit (1'000);
}