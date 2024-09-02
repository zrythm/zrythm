// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Project helper.
 */

#ifndef __TEST_HELPERS_PROJECT_H__
#define __TEST_HELPERS_PROJECT_H__

#include "zrythm-test-config.h"

#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/engine_dummy.h"
#include "dsp/marker_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_note.h"
#include "dsp/recording_manager.h"
#include "dsp/region.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "project/project_init_flow_manager.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "tests/helpers/zrythm_helper.h"

#include "doctest_wrapper.h"

/**
 * @addtogroup tests
 *
 * @{
 */

/** MidiNote value to use. */
constexpr int MN_VAL = 78;
/** MidiNote velocity to use. */
constexpr int MN_VEL = 23;

/** First AP value. */
constexpr float AP_VAL1 = 0.6f;
/** Second AP value. */
constexpr float AP_VAL2 = 0.9f;

/** Marker name. */
constexpr const char * MARKER_NAME = "Marker name";

constexpr auto MUSICAL_SCALE_TYPE = MusicalScale::Type::Ionian;
constexpr auto MUSICAL_SCALE_ROOT = MusicalNote::A;

constexpr int MOVE_TICKS = 400;

constexpr int TOTAL_TL_SELECTIONS = 6;

constexpr const char * MIDI_REGION_NAME = "Midi region";
constexpr const char * AUDIO_REGION_NAME = "Audio region";
constexpr const char * MIDI_TRACK_NAME = "Midi track";
constexpr const char * AUDIO_TRACK_NAME = "Audio track";

/* initial positions */
constexpr int MIDI_REGION_LANE = 2;
constexpr int AUDIO_REGION_LANE = 3;

/* target positions */
constexpr const char * TARGET_MIDI_TRACK_NAME = "Target midi tr";
constexpr const char * TARGET_AUDIO_TRACK_NAME = "Target audio tr";

/* TODO test moving lanes */
constexpr int TARGET_MIDI_REGION_LANE = 0;
constexpr int TARGET_AUDIO_REGION_LANE = 5;

fs::path
test_project_save ();

COLD void
test_project_reload (const fs::path &prj_file);

void
test_project_save_and_reload ();

/**
 * Stop dummy audio engine processing so we can
 * process manually.
 */
void
test_project_stop_dummy_engine ();

void
test_project_check_vs_original_state (
  Position * p1,
  Position * p2,
  int        check_selections);

fs::path
test_project_save ()
{
  /* save the project */
  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, 0, 0, false));
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
  REQUIRE_NONEMPTY (prj_file);

  /* recreate the recording manager to drop any events */
  gZrythm->recording_manager_ = std::make_unique<RecordingManager> ();

  /* reload it */
  test_project_reload (prj_file);
}

/**
 * Bootstraps the test with test data.
 */
class BootstrapTimelineFixture : public ZrythmFixture
{
public:
  BootstrapTimelineFixture ()
  {
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
      REQUIRE_NONEMPTY (midi_track->name_);
      const auto midi_track_name_hash = midi_track->get_name_hash ();
      REQUIRE_NE (midi_track_name_hash, 0);
      auto mr = std::make_shared<MidiRegion> (
        p1_, p2_, midi_track_name_hash, MIDI_REGION_LANE, 0);
      REQUIRE_NOTHROW (
        midi_track->add_region (mr, nullptr, MIDI_REGION_LANE, true, false));
      mr->set_name (MIDI_REGION_NAME, false);
      REQUIRE_EQ (mr->id_.track_name_hash_, midi_track_name_hash);
      REQUIRE_EQ (mr->id_.lane_pos_, MIDI_REGION_LANE);
      REQUIRE_EQ (mr->id_.type_, RegionType::Midi);
      TL_SELECTIONS->add_object_ref (mr);

      /* add a midi note to the region */
      auto mn = std::make_shared<MidiNote> (mr->id_, p1_, p2_, MN_VAL, MN_VEL);
      mr->append_object (mn);
      REQUIRE_EQ (mn->region_id_, mr->id_);
      MIDI_SELECTIONS->add_object_ref (mn);
    }

    {
      /* Create and add an automation region with 2 AutomationPoint's */
      auto at = P_MASTER_TRACK->channel_->get_automation_track (
        PortIdentifier::Flags::StereoBalance);
      const auto master_track_name_hash = P_MASTER_TRACK->get_name_hash ();
      auto       automation_r = std::make_shared<AutomationRegion> (
        p1_, p2_, master_track_name_hash, at->index_, 0);
      REQUIRE_NOTHROW (
        P_MASTER_TRACK->add_region (automation_r, at, 0, true, false));
      REQUIRE_EQ (automation_r->id_.track_name_hash_, master_track_name_hash);
      REQUIRE_EQ (automation_r->id_.at_idx_, at->index_);
      REQUIRE_EQ (automation_r->id_.type_, RegionType::Automation);
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
      REQUIRE_NOTHROW (
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
      REQUIRE_EQ (m->marker_track_index_, 2);
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
          AUDIO_TRACK_NAME, TRACKLIST->tracks_.size (),
          AUDIO_ENGINE->sample_rate_),
        false, false);
      REQUIRE_SIZE_EQ (P_MASTER_TRACK->children_, 1);
      auto       audio_file_path = fs::path (TESTS_SRCDIR) / "test.wav";
      const auto track_name_hash = audio_track->get_name_hash ();
      std::shared_ptr<AudioRegion> r;
      REQUIRE_NOTHROW (
        r = std::make_shared<AudioRegion> (
          -1, audio_file_path.string (), true, nullptr, 0, std::nullopt, 0,
          (BitDepth) 0, p1_, track_name_hash, AUDIO_REGION_LANE, 0));
      auto clip = r->get_clip ();
      REQUIRE_GT (clip->num_frames_, 151000);
      REQUIRE_LT (clip->num_frames_, 152000);
      REQUIRE_NOTHROW (
        audio_track->add_region (r, nullptr, AUDIO_REGION_LANE, true, false));
      r->set_name (AUDIO_REGION_NAME, false);
      REQUIRE_EQ (r->id_.track_name_hash_, track_name_hash);
      REQUIRE_EQ (r->id_.lane_pos_, AUDIO_REGION_LANE);
      REQUIRE_EQ (r->id_.type_, RegionType::Audio);
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
    REQUIRE_SIZE_EQ (P_MASTER_TRACK->children_, 2);

    AUDIO_ENGINE->update_frames_per_tick (
      P_TEMPO_TRACK->get_beats_per_bar (), P_TEMPO_TRACK->get_current_bpm (),
      AUDIO_ENGINE->sample_rate_, true, true, false);

    ROUTER->recalc_graph (false);

    AUDIO_ENGINE->resume (state);

    REQUIRE_SIZE_EQ (TL_SELECTIONS->objects_, 6);
    test_project_save_and_reload ();
    REQUIRE_SIZE_EQ (TL_SELECTIONS->objects_, 6);
    REQUIRE_SIZE_EQ (P_MASTER_TRACK->children_, 2);
  }

  /**
   * Checks that the objects are back to their original state.
   * @param check_selections Also checks that the selections are back to where
   * they were.
   */
  void check_vs_original_state (bool check_selections)
  {
    bool after_reload = false;

check_vs_orig_state:

    if (check_selections)
      {
        REQUIRE_EQ (TL_SELECTIONS->get_num_objects<Region> (), 4);
        REQUIRE_EQ (TL_SELECTIONS->get_num_objects<Marker> (), 1);
        REQUIRE_EQ (TL_SELECTIONS->get_num_objects<ScaleObject> (), 1);
      }

    auto midi_track = TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
    REQUIRE_NONNULL (midi_track);
    auto audio_track =
      TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
    REQUIRE_NONNULL (audio_track);

    Position p1_before_move = p1_;
    Position p2_before_move = p2_;
    p1_before_move.add_ticks (-MOVE_TICKS);
    p2_before_move.add_ticks (-MOVE_TICKS);

    { /* check midi region */
      REQUIRE_SIZE_EQ (midi_track->lanes_[MIDI_REGION_LANE]->regions_, 1);
      auto r = midi_track->lanes_[MIDI_REGION_LANE]->regions_[0];
      REQUIRE_POSITION_EQ (r->pos_, p1_);
      REQUIRE_POSITION_EQ (r->end_pos_, p2_);
      REQUIRE_SIZE_EQ (r->midi_notes_, 1);
      auto mn = r->midi_notes_[0];
      REQUIRE_EQ (mn->val_, MN_VAL);
      REQUIRE_EQ (mn->vel_->vel_, MN_VEL);
      REQUIRE_POSITION_EQ (mn->pos_, p1_);
      REQUIRE_POSITION_EQ (mn->end_pos_, p2_);
      REQUIRE_EQ (mn->region_id_, r->id_);
    }

    {
      /* check audio region */
      REQUIRE_SIZE_EQ (audio_track->lanes_[AUDIO_REGION_LANE]->regions_, 1);
      auto r = audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0];
      REQUIRE_POSITION_EQ (r->pos_, p1_);
    }

    {
      /* check automation region */
      auto at = P_MASTER_TRACK->channel_->get_automation_track (
        PortIdentifier::Flags::StereoBalance);
      REQUIRE_NONNULL (at);
      REQUIRE_SIZE_EQ (at->regions_, 1);
      auto r = at->regions_.at (0);
      REQUIRE_POSITION_EQ (r->pos_, p1_);
      REQUIRE_POSITION_EQ (r->end_pos_, p2_);
      REQUIRE_SIZE_EQ (r->aps_, 2);
      auto ap = r->aps_.at (0);
      REQUIRE_POSITION_EQ (ap->pos_, p1_);
      REQUIRE_FLOAT_NEAR (ap->fvalue_, AP_VAL1, 0.000001f);
      ap = r->aps_[1];
      REQUIRE_POSITION_EQ (ap->pos_, p2_);
      REQUIRE_FLOAT_NEAR (ap->fvalue_, AP_VAL2, 0.000001f);
    }

    {
      /* check marker */
      REQUIRE_SIZE_EQ (P_MARKER_TRACK->markers_, 3);
      auto m = P_MARKER_TRACK->markers_[0];
      REQUIRE_NONNULL (m);
      REQUIRE_EQ (m->name_, "[start]");
      m = P_MARKER_TRACK->markers_[2];
      REQUIRE_POSITION_EQ (m->pos_, p1_);
      REQUIRE_EQ (m->name_, MARKER_NAME);
    }

    {
      /* check scale object */
      REQUIRE_SIZE_EQ (P_CHORD_TRACK->scales_, 1);
      auto s = P_CHORD_TRACK->scales_[0];
      REQUIRE_POSITION_EQ (s->pos_, p1_);
      REQUIRE_EQ (s->scale_.type_, MUSICAL_SCALE_TYPE);
      REQUIRE_EQ (s->scale_.root_key_, MUSICAL_SCALE_ROOT);
    }

    /* save the project and reopen it. some callers
     * undo after this step so this checks if the undo
     * history works after reopening the project */
    if (!after_reload)
      {
        test_project_save_and_reload ();
        after_reload = true;
        goto check_vs_orig_state;
      }
  }

  virtual ~BootstrapTimelineFixture () = default;

public:
  Position p1_;
  Position p2_;
};

/**
 * Stop dummy audio engine processing so we can process manually.
 */
void
test_project_stop_dummy_engine ()
{
  AUDIO_ENGINE->dummy_audio_thread_->signalThreadShouldExit ();
  AUDIO_ENGINE->dummy_audio_thread_->waitForThreadToExit (1'000);
}

/**
 * @}
 */

#endif
