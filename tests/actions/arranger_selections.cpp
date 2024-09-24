// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/arranger_selections.h"
#include "actions/tracklist_selections.h"
#include "actions/transport_action.h"
#include "dsp/audio_track.h"
#include "dsp/automation_region.h"
#include "dsp/chord_track.h"
#include "dsp/marker_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_track.h"
#include "dsp/region.h"
#include "dsp/tempo_track.h"
#include "gui/backend/clipboard.h"
#include "project.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/gtest_wrapper.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/project_helper.h"

auto REQUIRE_OBJ_TRACK_NAME_HASH_MATCHES_TRACK =
  [] (const auto &obj, const Track &track) {
    using T = base_type<decltype (obj)>;
    if constexpr (std::derived_from<T, RegionImpl<T>>)
      {
        ASSERT_TRUE (obj->id_.track_name_hash_ == track.get_name_hash ());
      }
    else if constexpr (std::derived_from<T, RegionOwnedObjectImpl<T>>)
      {
        ASSERT_TRUE (obj->region_id_.track_name_hash_ == track.get_name_hash ());
      }
    else
      {
        [[maybe_unused]] typedef typename T::something_made_up X;
      }
  };

auto perform_create = [] (const auto &selections) {
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*selections));
};

auto perform_delete = [] (const auto &selections) {
  UNDO_MANAGER->perform (
    std::make_unique<DeleteArrangerSelectionsAction> (*selections));
};

auto perform_split = [] (const auto &selections, const Position &pos) {
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::SplitAction> (*selections, pos));
};

#if 0
Position p1, p2;

/**
 * Bootstraps the test with test data.
 */
static void
rebootstrap_timeline (void)
{
  test_project_rebootstrap_timeline (&p1, &p2);
}
#endif

class ArrangerSelectionsFixture : public BootstrapTimelineFixture
{
public:
  void select_audio_and_midi_regions_only ()
  {
    TL_SELECTIONS->clear ();
    ASSERT_EMPTY (TL_SELECTIONS->objects_);

    auto midi_track = TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
    ASSERT_NONNULL (midi_track);
    TL_SELECTIONS->add_object_ref (
      midi_track->lanes_[MIDI_REGION_LANE]->regions_[0]);
    ASSERT_SIZE_EQ (TL_SELECTIONS->objects_, 1);
    ASSERT_TRUE (TL_SELECTIONS->contains_only_regions ());
    ASSERT_TRUE (TL_SELECTIONS->contains_only_region_types (RegionType::Midi));
    auto audio_track =
      TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
    ASSERT_NONNULL (audio_track);
    TL_SELECTIONS->add_object_ref (
      audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0]);
    ASSERT_SIZE_EQ (TL_SELECTIONS->objects_, 2);
  }

  /**
   * Check if the undo stack has a single element.
   */
  void check_has_single_undo ()
  {
    ASSERT_SIZE_EQ (*UNDO_MANAGER->undo_stack_, 1);
    ASSERT_EMPTY (*UNDO_MANAGER->redo_stack_);
  }

  /**
   * Check if the redo stack has a single element.
   */
  void check_has_single_redo ()
  {
    ASSERT_SIZE_EQ (*UNDO_MANAGER->redo_stack_, 1);
    ASSERT_EMPTY (*UNDO_MANAGER->undo_stack_);
  }

  /**
   * Checks that the objects are deleted.
   *
   * @param creating Whether this is part of a create test.
   */
  void check_timeline_objects_deleted (bool creating)
  {
    ASSERT_EMPTY (TL_SELECTIONS->objects_);

    auto midi_track = TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
    ASSERT_NONNULL (midi_track);
    auto audio_track =
      TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
    ASSERT_NONNULL (audio_track);

    /* check midi region */
    ASSERT_SIZE_EQ (midi_track->lanes_, 1);
    ASSERT_EMPTY (midi_track->lanes_[0]->regions_);

    /* check audio region */
    ASSERT_SIZE_EQ (audio_track->lanes_, 1);
    ASSERT_EMPTY (audio_track->lanes_[0]->regions_);

    /* check automation region */
    auto at = P_MASTER_TRACK->channel_->get_automation_track (
      PortIdentifier::Flags::StereoBalance);
    ASSERT_NONNULL (at);
    ASSERT_EMPTY (at->regions_);

    /* check marker */
    ASSERT_SIZE_EQ (P_MARKER_TRACK->markers_, 2);

    /* check scale object */
    ASSERT_EMPTY (P_CHORD_TRACK->scales_);

    if (creating)
      {
        check_has_single_redo ();
      }
    else
      {
        check_has_single_undo ();
      }
  }

  /**
   * Checks the objects after moving.
   *
   * @param new_tracks Whether objects were moved to
   *   new tracks or in the same track.
   */
  void check_after_move_timeline (bool new_tracks)
  {
    /* check */
    ASSERT_EQ (
      TL_SELECTIONS->get_num_objects (), new_tracks ? 2 : TOTAL_TL_SELECTIONS);

    /* check that undo/redo stacks have the correct counts (1 and 0) */
    check_has_single_undo ();
    ASSERT_EQ (
      dynamic_cast<ArrangerSelectionsAction *> (
        UNDO_MANAGER->undo_stack_->peek ())
        ->delta_tracks_,
      new_tracks ? 2 : 0);

    /* get tracks */
    auto midi_track_before =
      TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
    ASSERT_NONNULL (midi_track_before);
    ASSERT_GT (midi_track_before->pos_, 0);
    auto audio_track_before =
      TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
    ASSERT_NONNULL (audio_track_before);
    ASSERT_GT (audio_track_before->pos_, 0);
    auto midi_track_after =
      TRACKLIST->find_track_by_name<MidiTrack> (TARGET_MIDI_TRACK_NAME);
    ASSERT_NONNULL (midi_track_after);
    ASSERT_GT (midi_track_after->pos_, midi_track_before->pos_);
    auto audio_track_after =
      TRACKLIST->find_track_by_name<AudioTrack> (TARGET_AUDIO_TRACK_NAME);
    ASSERT_NONNULL (audio_track_after);
    ASSERT_GT (audio_track_after->pos_, audio_track_before->pos_);

    auto require_correct_track = [&] (auto &region, auto &expected) {
      const auto actual = region->get_track ();
      ASSERT_EQ (actual, expected);
      ASSERT_EQ (region->id_.track_name_hash_, expected->get_name_hash ());
    };

    auto p1_after_move = p1_;
    auto p2_after_move = p2_;
    p1_after_move.add_ticks (MOVE_TICKS);
    p2_after_move.add_ticks (MOVE_TICKS);

    {
      /* check midi region */
      const auto mr = [&] () {
        const auto track = new_tracks ? midi_track_after : midi_track_before;
        return track->lanes_[MIDI_REGION_LANE]->regions_[0];
      }();

      ASSERT_TRUE (mr->is_region ());
      ASSERT_TRUE (mr->is_midi ());

      ASSERT_POSITION_EQ (mr->pos_, p1_after_move);
      ASSERT_POSITION_EQ (mr->end_pos_, p2_after_move);
      require_correct_track (
        mr, new_tracks ? midi_track_after : midi_track_before);
      ASSERT_SIZE_EQ (mr->midi_notes_, 1);
      const auto mn = mr->midi_notes_[0];
      ASSERT_EQ (mn->val_, MN_VAL);
      ASSERT_EQ (mn->vel_->vel_, MN_VEL);
      ASSERT_POSITION_EQ (mn->pos_, p1_);
      ASSERT_POSITION_EQ (mn->end_pos_, p2_);
      ASSERT_EQ (mn->region_id_, mr->id_);
    }

    {
      /* check audio region */
      const auto regions = TL_SELECTIONS->get_objects_of_type<Region> ();
      const auto r =
        dynamic_pointer_cast<AudioRegion> (regions.at (new_tracks ? 1 : 3));
      ASSERT_NONNULL (r);
      ASSERT_TRUE (r->is_region ());
      ASSERT_TRUE (r->is_audio ());
      ASSERT_POSITION_EQ (r->pos_, p1_after_move);
      require_correct_track (
        r, new_tracks ? audio_track_after : audio_track_before);
    }

    if (!new_tracks)
      {
        {
          /* check automation region */
          const auto regions = TL_SELECTIONS->get_objects_of_type<Region> ();
          const auto r = dynamic_pointer_cast<AutomationRegion> (regions.at (1));
          ASSERT_POSITION_EQ (r->pos_, p1_after_move);
          ASSERT_POSITION_EQ (r->end_pos_, p2_after_move);
          ASSERT_SIZE_EQ (r->aps_, 2);
          {
            const auto &ap = r->aps_[0];
            ASSERT_POSITION_EQ (ap->pos_, p1_);
            ASSERT_NEAR (ap->fvalue_, AP_VAL1, 0.000001f);
          }
          {
            const auto &ap = r->aps_[1];
            ASSERT_POSITION_EQ (ap->pos_, p2_);
            ASSERT_NEAR (ap->fvalue_, AP_VAL2, 0.000001f);
          }
        }

        /* check marker */
        auto markers = TL_SELECTIONS->get_objects_of_type<Marker> ();
        ASSERT_SIZE_EQ (markers, 1);
        auto m = markers[0];
        ASSERT_POSITION_EQ (m->pos_, p1_after_move);
        ASSERT_EQ (m->name_, MARKER_NAME);

        /* check scale object */
        auto scales = TL_SELECTIONS->get_objects_of_type<ScaleObject> ();
        ASSERT_SIZE_EQ (scales, 1);
        auto s = scales[0];
        ASSERT_POSITION_EQ (s->pos_, p1_after_move);
        ASSERT_EQ (s->scale_.type_, MUSICAL_SCALE_TYPE);
        ASSERT_EQ (s->scale_.root_key_, MUSICAL_SCALE_ROOT);
      }
  }

  /**
   * @param new_tracks Whether midi/audio regions
   *   were moved to new tracks.
   * @param link Whether this is a link action.
   */
  void check_after_duplicate_timeline (bool new_tracks, bool link)
  {
    /* check */
    EXPECT_EQ (
      TL_SELECTIONS->get_num_objects (), new_tracks ? 2 : TOTAL_TL_SELECTIONS);

    const auto * midi_track =
      TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
    EXPECT_NONNULL (midi_track);
    const auto * audio_track =
      TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
    EXPECT_NONNULL (audio_track);
    const auto * new_midi_track =
      TRACKLIST->find_track_by_name<MidiTrack> (TARGET_MIDI_TRACK_NAME);
    EXPECT_NONNULL (midi_track);
    const auto * new_audio_track =
      TRACKLIST->find_track_by_name<AudioTrack> (TARGET_AUDIO_TRACK_NAME);
    EXPECT_NONNULL (audio_track);

    /* check prev midi region */
    if (new_tracks)
      {
        EXPECT_SIZE_EQ (midi_track->lanes_.at (MIDI_REGION_LANE)->regions_, 1);
        EXPECT_SIZE_EQ (
          new_midi_track->lanes_.at (MIDI_REGION_LANE)->regions_, 1);
      }
    else
      {
        EXPECT_SIZE_EQ (midi_track->lanes_.at (MIDI_REGION_LANE)->regions_, 2);
      }

    auto       mr = midi_track->lanes_[MIDI_REGION_LANE]->regions_[0];
    const auto p1_before_move = p1_;
    const auto p2_before_move = p2_;
    EXPECT_POSITION_EQ (mr->pos_, p1_before_move);
    EXPECT_POSITION_EQ (mr->end_pos_, p2_before_move);
    REQUIRE_OBJ_TRACK_NAME_HASH_MATCHES_TRACK (mr, *midi_track);
    EXPECT_EQ (mr->id_.lane_pos_, MIDI_REGION_LANE);
    EXPECT_EQ (mr->id_.idx_, 0);
    EXPECT_SIZE_EQ (mr->midi_notes_, 1);
    {
      const auto &mn = mr->midi_notes_.at (0);
      EXPECT_EQ (mn->region_id_, mr->id_);
      EXPECT_EQ (mn->val_, MN_VAL);
      EXPECT_EQ (mn->vel_->vel_, MN_VEL);
      EXPECT_POSITION_EQ (mn->pos_, p1_);
      EXPECT_POSITION_EQ (mn->end_pos_, p2_);
    }
    int link_group = mr->id_.link_group_;
    if (link)
      {
        EXPECT_GT (mr->id_.link_group_, -1);
      }

    /* check new midi region */
    if (new_tracks)
      {
        mr = new_midi_track->lanes_[MIDI_REGION_LANE]->regions_[0];
      }
    else
      {
        mr = midi_track->lanes_[MIDI_REGION_LANE]->regions_[1];
      }
    auto p1_after_move = p1_;
    auto p2_after_move = p2_;
    p1_after_move.add_ticks (MOVE_TICKS);
    p2_after_move.add_ticks (MOVE_TICKS);
    EXPECT_POSITION_EQ (mr->pos_, p1_after_move);
    EXPECT_POSITION_EQ (mr->end_pos_, p2_after_move);
    if (new_tracks)
      {
        REQUIRE_OBJ_TRACK_NAME_HASH_MATCHES_TRACK (mr, *new_midi_track);
        EXPECT_EQ (mr->id_.idx_, 0);
      }
    else
      {
        REQUIRE_OBJ_TRACK_NAME_HASH_MATCHES_TRACK (mr, *midi_track);
        EXPECT_EQ (mr->id_.idx_, 1);
      }
    EXPECT_EQ (mr->id_.lane_pos_, MIDI_REGION_LANE);
    EXPECT_SIZE_EQ (mr->midi_notes_, 1);
    REQUIRE_OBJ_TRACK_NAME_HASH_MATCHES_TRACK (
      mr, new_tracks ? *new_midi_track : *midi_track);
    EXPECT_EQ (mr->id_.lane_pos_, MIDI_REGION_LANE);
    {
      const auto &mn = mr->midi_notes_[0];
      EXPECT_EQ (mn->region_id_, mr->id_);
      EXPECT_EQ (mn->val_, MN_VAL);
      EXPECT_EQ (mn->vel_->vel_, MN_VEL);
      EXPECT_POSITION_EQ (mn->pos_, p1_);
      EXPECT_POSITION_EQ (mn->end_pos_, p2_);
    }
    if (link)
      {
        EXPECT_EQ (mr->id_.link_group_, link_group);
      }

    /* check prev audio region */
    if (new_tracks)
      {
        EXPECT_SIZE_EQ (audio_track->lanes_[AUDIO_REGION_LANE]->regions_, 1);
        EXPECT_SIZE_EQ (new_audio_track->lanes_[AUDIO_REGION_LANE]->regions_, 1);
      }
    else
      {
        EXPECT_SIZE_EQ (audio_track->lanes_[AUDIO_REGION_LANE]->regions_, 2);
      }
    auto ar = audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0];
    EXPECT_POSITION_EQ (ar->pos_, p1_before_move);
    REQUIRE_OBJ_TRACK_NAME_HASH_MATCHES_TRACK (ar, *audio_track);
    EXPECT_EQ (ar->id_.idx_, 0);
    EXPECT_EQ (ar->id_.lane_pos_, AUDIO_REGION_LANE);
    link_group = ar->id_.link_group_;
    if (link)
      {
        EXPECT_GT (ar->id_.link_group_, -1);
      }

    /* check new audio region */
    if (new_tracks)
      {
        ar = new_audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0];
      }
    else
      {
        ar = audio_track->lanes_[AUDIO_REGION_LANE]->regions_[1];
      }
    EXPECT_POSITION_EQ (ar->pos_, p1_after_move);
    EXPECT_EQ (ar->id_.lane_pos_, AUDIO_REGION_LANE);
    REQUIRE_OBJ_TRACK_NAME_HASH_MATCHES_TRACK (
      ar, new_tracks ? *new_audio_track : *audio_track);
    if (link)
      {
        EXPECT_EQ (ar->id_.link_group_, link_group);
      }

    if (!new_tracks)
      {
        /* check automation region */
        auto * const at = P_MASTER_TRACK->channel_->get_automation_track (
          PortIdentifier::Flags::StereoBalance);
        EXPECT_NONNULL (at);
        EXPECT_SIZE_EQ (at->regions_, 2);

        auto check_automation_region_contents =
          [&] (const auto &automation_region) {
            EXPECT_SIZE_EQ (automation_region->aps_, 2);
            const auto &ap1 = automation_region->aps_[0];
            EXPECT_POSITION_EQ (ap1->pos_, p1_);
            EXPECT_NEAR (ap1->fvalue_, AP_VAL1, 0.000001f);
            const auto &ap2 = automation_region->aps_[1];
            EXPECT_POSITION_EQ (ap2->pos_, p2_);
            EXPECT_NEAR (ap2->fvalue_, AP_VAL2, 0.000001f);
          };

        {
          const auto &r = at->regions_.at (0);
          EXPECT_POSITION_EQ (r->pos_, p1_before_move);
          EXPECT_POSITION_EQ (r->end_pos_, p2_before_move);
          check_automation_region_contents (r);
        }
        {
          const auto &r = at->regions_.at (1);
          EXPECT_POSITION_EQ (r->pos_, p1_after_move);
          EXPECT_POSITION_EQ (r->end_pos_, p2_after_move);
          check_automation_region_contents (r);
        }

        /* check marker */
        EXPECT_SIZE_EQ (P_MARKER_TRACK->markers_, 4);
        auto m = P_MARKER_TRACK->markers_.at (2);
        EXPECT_POSITION_EQ (m->pos_, p1_before_move);
        EXPECT_EQ (m->name_, MARKER_NAME);
        m = P_MARKER_TRACK->markers_[3];
        EXPECT_POSITION_EQ (m->pos_, p1_after_move);
        EXPECT_EQ (m->name_, MARKER_NAME);

        /* check scale object */
        EXPECT_SIZE_EQ (P_CHORD_TRACK->scales_, 2);
        auto s = P_CHORD_TRACK->scales_[0];
        EXPECT_POSITION_EQ (s->pos_, p1_before_move);
        EXPECT_EQ (s->scale_.type_, MUSICAL_SCALE_TYPE);
        EXPECT_EQ (s->scale_.root_key_, MUSICAL_SCALE_ROOT);
        s = P_CHORD_TRACK->scales_.at (1);
        EXPECT_POSITION_EQ (s->pos_, p1_after_move);
        EXPECT_EQ (s->scale_.type_, MUSICAL_SCALE_TYPE);
        EXPECT_EQ (s->scale_.root_key_, MUSICAL_SCALE_ROOT);
      }
  }
};

TEST_F (ArrangerSelectionsFixture, DuplicateTimeline)
{
  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      const int track_diff = i == 0 ? 0 : 2;
      if (track_diff != 0)
        {
          select_audio_and_midi_regions_only ();
          auto * midi_track =
            TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
          auto * audio_track =
            TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
          auto * new_midi_track =
            TRACKLIST->find_track_by_name<MidiTrack> (TARGET_MIDI_TRACK_NAME);
          auto * new_audio_track =
            TRACKLIST->find_track_by_name<AudioTrack> (TARGET_AUDIO_TRACK_NAME);
          midi_track->lanes_.at (MIDI_REGION_LANE)
            ->regions_.front ()
            ->move_to_track (new_midi_track, -1, -1);
          audio_track->lanes_.at (AUDIO_REGION_LANE)
            ->regions_.front ()
            ->move_to_track (new_audio_track, -1, -1);
        }
      /* do move ticks */
      TL_SELECTIONS->add_ticks (MOVE_TICKS);

      /* do duplicate */
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
          *TL_SELECTIONS, false, MOVE_TICKS, track_diff, 0, nullptr,
          F_ALREADY_MOVED));

      /* check */
      check_after_duplicate_timeline (i != 0, false);

      /* undo and check that the objects are at their original state*/
      UNDO_MANAGER->undo ();
      check_vs_original_state (false);
      check_has_single_redo ();

      /* redo and check that the objects are moved
       * again */
      UNDO_MANAGER->redo ();
      check_after_duplicate_timeline (i != 0, false);

      /* undo again to prepare for next test */
      UNDO_MANAGER->undo ();
      check_vs_original_state (false);
      check_has_single_redo ();
    }
}

TEST_F (ArrangerSelectionsFixture, CopyAudioRegionAndPasteAfterChangingBPM)
{
  TL_SELECTIONS->clear ();
  ASSERT_EMPTY (TL_SELECTIONS->objects_);
  auto audio_track =
    TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
  ASSERT_NONNULL (audio_track);
  TL_SELECTIONS->add_object_ref (
    audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0]);
  ASSERT_SIZE_EQ (TL_SELECTIONS->objects_, 1);
  audio_track->select (true, true, false);

  Clipboard clipboard (*TL_SELECTIONS);
  if (clipboard.arranger_sel_)
    {
      auto tl_sel =
        dynamic_cast<TimelineSelections *> (clipboard.arranger_sel_.get ());
      tl_sel->set_vis_track_indices ();
    }
  auto serialized = clipboard.serialize_to_json_string ();

  UNDO_MANAGER->perform (std::make_unique<TransportAction> (
    P_TEMPO_TRACK->get_current_bpm (), 110.f, false));

  Clipboard new_clipboard;
  clipboard.deserialize_from_json_string (serialized.c_str ());

  auto sel = clipboard.get_selections ();
  ASSERT_NONNULL (sel);
  sel->post_deserialize ();
  ASSERT_TRUE (sel->can_be_pasted ());
  sel->paste_to_pos (PLAYHEAD, true);
}

TEST_F (ArrangerSelectionsFixture, DuplicateAutomationRegion)
{
  auto at = P_MASTER_TRACK->channel_->get_automation_track (
    PortIdentifier::Flags::StereoBalance);
  ASSERT_NONNULL (at);
  ASSERT_SIZE_EQ (at->regions_, 1);

  Position start_pos, end_pos;
  end_pos.set_to_bar (4);
  auto r1 = at->regions_[0];

  auto ap = std::make_shared<AutomationPoint> (0.5f, 0.5f, start_pos);
  r1->append_object (ap);
  ap->select (true, false, false);
  perform_create (AUTOMATION_SELECTIONS);
  start_pos.add_frames (14);
  ap = std::make_shared<AutomationPoint> (0.6f, 0.6f, start_pos);
  r1->append_object (ap);
  ap->select (true, false, false);
  perform_create (AUTOMATION_SELECTIONS);

  constexpr float curviness_after = 0.8f;
  ap = r1->aps_[0];
  ap->select (true, false, false);
  auto before = AUTOMATION_SELECTIONS->clone_unique ();
  ap->curve_opts_.curviness_ = curviness_after;
  UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
    *before, AUTOMATION_SELECTIONS.get (),
    ArrangerSelectionsAction::EditType::Primitive, true));

  ap = r1->aps_[0];
  ASSERT_NEAR (ap->curve_opts_.curviness_, curviness_after, 0.00001f);

  UNDO_MANAGER->undo ();
  ap = r1->aps_[0];
  ASSERT_NEAR (ap->curve_opts_.curviness_, 0.f, 0.00001f);

  UNDO_MANAGER->redo ();
  ap = r1->aps_[0];
  ASSERT_NEAR (ap->curve_opts_.curviness_, curviness_after, 0.00001f);

  r1->select (true, false, false);

  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, false, MOVE_TICKS, 0, 0, nullptr, F_NOT_ALREADY_MOVED));

  r1 = at->regions_[1];
  ap = r1->aps_[0];
  ASSERT_NEAR (ap->curve_opts_.curviness_, curviness_after, 0.00001f);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  r1 = at->regions_[1];
  ap = r1->aps_[0];
  ASSERT_NEAR (ap->curve_opts_.curviness_, curviness_after, 0.00001f);
}

TEST_F (ArrangerSelectionsFixture, LinkTimeline)
{
  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      std::unique_ptr<TimelineSelections> sel_before;
      check_vs_original_state (false);

      int track_diff = i != 0 ? 2 : 0;
      if (track_diff != 0)
        {
          select_audio_and_midi_regions_only ();
          sel_before = TL_SELECTIONS->clone_unique ();

          auto * midi_track =
            TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
          auto * audio_track =
            TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
          auto * new_midi_track =
            TRACKLIST->find_track_by_name<MidiTrack> (TARGET_MIDI_TRACK_NAME);
          auto * new_audio_track =
            TRACKLIST->find_track_by_name<AudioTrack> (TARGET_AUDIO_TRACK_NAME);
          midi_track->lanes_[MIDI_REGION_LANE]->regions_[0]->move_to_track (
            new_midi_track, -1, -1);
          audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0]->move_to_track (
            new_audio_track, -1, -1);
        }
      else
        {
          sel_before = TL_SELECTIONS->clone_unique ();
        }

      /* do move ticks */
      TL_SELECTIONS->add_ticks (MOVE_TICKS);

      /* do link */
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::LinkAction> (
          *sel_before, *TL_SELECTIONS, MOVE_TICKS, track_diff, 0, false));

      /* check */
      check_after_duplicate_timeline (track_diff > 0, true);

      /* undo and check that the objects are at their original state*/
      UNDO_MANAGER->undo ();
      check_vs_original_state (false);
      check_has_single_redo ();

      /* redo and check that the objects are moved
       * again */
      UNDO_MANAGER->redo ();
      check_after_duplicate_timeline (i, true);

      /* undo again to prepare for next test */
      UNDO_MANAGER->undo ();
      check_vs_original_state (false);
      check_has_single_redo ();

      ASSERT_EMPTY (REGION_LINK_GROUP_MANAGER.groups_);
    }
}

TEST_F (ArrangerSelectionsFixture, LinkAndDelete)
{
  ASSERT_EMPTY (REGION_LINK_GROUP_MANAGER.groups_);

  auto midi_track = TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
  constexpr auto lane_idx = 2;
  auto          &lane = midi_track->lanes_[lane_idx];
  auto          &r = midi_track->lanes_[lane_idx]->regions_[0];
  r->select (true, false, false);

  auto move_and_perform_link = [] () {
    auto sel_before = TL_SELECTIONS->clone_unique ();
    TL_SELECTIONS->add_ticks (MOVE_TICKS);
    UNDO_MANAGER->perform (std::make_unique<ArrangerSelectionsAction::LinkAction> (
      *sel_before, *TL_SELECTIONS, MOVE_TICKS, 0, 0, true));
  };

  /* create linked object */
  move_and_perform_link ();
  r = midi_track->lanes_[lane_idx]->regions_[0];
  auto &r2 = midi_track->lanes_[lane_idx]->regions_[1];
  ASSERT_EQ (r->id_.link_group_, 0);
  ASSERT_EQ (r2->id_.link_group_, 0);
  ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 1);

  /* create another linked object */
  r2->select (true, false, false);
  move_and_perform_link ();
  r = midi_track->lanes_[lane_idx]->regions_.at (0);
  r2 = midi_track->lanes_[lane_idx]->regions_.at (1);
  auto &r3 = midi_track->lanes_[lane_idx]->regions_.at (2);
  ASSERT_EQ (r->id_.link_group_, 0);
  ASSERT_EQ (r2->id_.link_group_, 0);
  ASSERT_EQ (r3->id_.link_group_, 0);
  ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 1);

  /* create a separate object (no link group) */
  auto pos = r3->pos_;
  pos.add_bars (2);
  auto end_pos = r3->end_pos_;
  end_pos.add_bars (2);
  auto r4 = std::make_shared<MidiRegion> (
    pos, end_pos, midi_track->get_name_hash (), 0, lane->regions_.size ());
  midi_track->add_region (r4, nullptr, lane->pos_, true, false);
  r4->select (true, false, true);
  perform_create (TL_SELECTIONS);

  /* create a linked object (new link group with r4/r5) */
  r4->select (true, false, false);
  move_and_perform_link ();
  r = midi_track->lanes_[lane_idx]->regions_[0];
  r2 = midi_track->lanes_[lane_idx]->regions_[1];
  r3 = midi_track->lanes_[lane_idx]->regions_[2];
  r4 = midi_track->lanes_[lane_idx]->regions_[3];
  auto &r5 = midi_track->lanes_[lane_idx]->regions_.at (4);
  ASSERT_EQ (r->id_.link_group_, 0);
  ASSERT_EQ (r2->id_.link_group_, 0);
  ASSERT_EQ (r3->id_.link_group_, 0);
  ASSERT_EQ (r4->id_.link_group_, 1);
  ASSERT_EQ (r5->id_.link_group_, 1);
  ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 2);

  /* delete the middle linked object */
  r2->select (true, false, false);
  perform_delete (TL_SELECTIONS);
  r = midi_track->lanes_[lane_idx]->regions_[0];
  r2 = midi_track->lanes_[lane_idx]->regions_[1];
  ASSERT_EQ (r->id_.link_group_, 0);
  ASSERT_EQ (r2->id_.link_group_, 0);
  ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 2);

  /* undo deleting middle linked object (in link group 0) */
  UNDO_MANAGER->undo ();
  r = midi_track->lanes_[lane_idx]->regions_[0];
  r2 = midi_track->lanes_[lane_idx]->regions_[1];
  r3 = midi_track->lanes_[lane_idx]->regions_[2];
  r4 = midi_track->lanes_[lane_idx]->regions_[3];
  r5 = midi_track->lanes_[lane_idx]->regions_[4];
  ASSERT_EQ (r->id_.link_group_, 0);
  ASSERT_EQ (r2->id_.link_group_, 0);
  ASSERT_EQ (r3->id_.link_group_, 0);
  ASSERT_EQ (r4->id_.link_group_, 1);
  ASSERT_EQ (r5->id_.link_group_, 1);
  ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 2);

  /* delete the first object in 2nd link group */
  r4->select (true, false, false);
  perform_delete (TL_SELECTIONS);
  r = midi_track->lanes_[lane_idx]->regions_[0];
  r2 = midi_track->lanes_[lane_idx]->regions_[1];
  r3 = midi_track->lanes_[lane_idx]->regions_[2];
  r5 = midi_track->lanes_[lane_idx]->regions_[3];
  ASSERT_EQ (r->id_.link_group_, 0);
  ASSERT_EQ (r2->id_.link_group_, 0);
  ASSERT_EQ (r3->id_.link_group_, 0);
  ASSERT_EQ (r5->id_.link_group_, -1);
  ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 1);

  /* undo deleting first object in 2nd link group */
  UNDO_MANAGER->undo ();
  r = midi_track->lanes_[lane_idx]->regions_[0];
  r2 = midi_track->lanes_[lane_idx]->regions_[1];
  r3 = midi_track->lanes_[lane_idx]->regions_[2];
  r4 = midi_track->lanes_[lane_idx]->regions_[3];
  r5 = midi_track->lanes_[lane_idx]->regions_[4];
  ASSERT_EQ (r->id_.link_group_, 0);
  ASSERT_EQ (r2->id_.link_group_, 0);
  ASSERT_EQ (r3->id_.link_group_, 0);
  ASSERT_EQ (r4->id_.link_group_, 1);
  /* FIXME the link group of the adjacent object is not restored */
  /*ASSERT_EQ (r5->id_.link_group_,  1);*/
  ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 2);

  /* redo and verify again */
  r = midi_track->lanes_[lane_idx]->regions_[0];
  r2 = midi_track->lanes_[lane_idx]->regions_[1];
  r3 = midi_track->lanes_[lane_idx]->regions_[2];
  r5 = midi_track->lanes_[lane_idx]->regions_[3];
  ASSERT_EQ (r->id_.link_group_, 0);
  ASSERT_EQ (r2->id_.link_group_, 0);
  ASSERT_EQ (r3->id_.link_group_, 0);
  ASSERT_EQ (r5->id_.link_group_, 1);
  ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 2);
}

TEST_F (ArrangerSelectionsFixture, LinkThenDuplicate)
{
  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      std::unique_ptr<TimelineSelections> sel_before;

      int track_diff = i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
          sel_before = TL_SELECTIONS->clone_unique ();

          auto midi_track =
            TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
          auto audio_track =
            TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
          auto new_midi_track =
            TRACKLIST->find_track_by_name<MidiTrack> (TARGET_MIDI_TRACK_NAME);
          auto new_audio_track =
            TRACKLIST->find_track_by_name<AudioTrack> (TARGET_AUDIO_TRACK_NAME);
          midi_track->lanes_[MIDI_REGION_LANE]->regions_[0]->move_to_track (
            new_midi_track, -1, -1);
          audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0]->move_to_track (
            new_audio_track, -1, -1);
        }
      else
        {
          sel_before = TL_SELECTIONS->clone_unique ();
        }

      /* do move ticks */
      TL_SELECTIONS->add_ticks (MOVE_TICKS);

      /* do link */
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::LinkAction> (
          *sel_before, *TL_SELECTIONS, MOVE_TICKS, i > 0 ? 2 : 0, 0,
          F_ALREADY_MOVED));

      /* check */
      check_after_duplicate_timeline (i, true);

      if (track_diff)
        {
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 2);
        }
      else
        {
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 4);
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_[2].ids_, 2);
        }

      REGION_LINK_GROUP_MANAGER.validate ();

      /* duplicate and check that the new objects are not links */
      if (track_diff)
        {
          auto midi_track =
            TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
          auto audio_track =
            TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
          auto new_midi_track =
            TRACKLIST->find_track_by_name<MidiTrack> (TARGET_MIDI_TRACK_NAME);
          auto new_audio_track =
            TRACKLIST->find_track_by_name<AudioTrack> (TARGET_AUDIO_TRACK_NAME);
          auto &mr = midi_track->lanes_[MIDI_REGION_LANE]->regions_[0];
          mr->move_to_track (new_midi_track, -1, -1);
          mr->select (true, false, false);
          auto ar = audio_track->lanes_[AUDIO_REGION_LANE]->regions_[0];
          ar->move_to_track (new_audio_track, -1, -1);
          ar->select (true, true, false);
        }
      else
        {
          select_audio_and_midi_regions_only ();
        }

      /* do move ticks */
      TL_SELECTIONS->add_ticks (MOVE_TICKS);

      if (track_diff)
        {
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 2);
        }
      else
        {
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 4);
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_[2].ids_, 2);
        }

      REGION_LINK_GROUP_MANAGER.validate ();

      /* do duplicate */
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
          *TL_SELECTIONS, false, MOVE_TICKS, i > 0 ? 2 : 0, 0, nullptr,
          F_ALREADY_MOVED));

      /* check that new objects have no links */
      for (auto &r : TL_SELECTIONS->get_objects_of_type<Region> ())
        {
          ASSERT_NONNULL (r);
          ASSERT_EQ (r->id_.link_group_, -1);
          ASSERT_FALSE (r->has_link_group ());
        }

      if (track_diff)
        {
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 2);
        }
      else
        {
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_, 4);
          ASSERT_SIZE_EQ (REGION_LINK_GROUP_MANAGER.groups_[2].ids_, 2);
        }

      test_project_save_and_reload ();

      /* add a midi note to a linked midi region */
      auto r = [&] () -> std::shared_ptr<MidiRegion> {
        auto track = TRACKLIST->find_track_by_name<MidiTrack> (
          track_diff ? TARGET_MIDI_TRACK_NAME : MIDI_TRACK_NAME);
        EXPECT_NONNULL (track);
        auto ret =
          track->lanes_[MIDI_REGION_LANE]->regions_.at (track_diff ? 0 : 1);
        EXPECT_TRUE (ret->is_midi ());
        EXPECT_TRUE (ret->has_link_group ());
        return ret;
      }();
      Position start, end;
      start.set_to_bar (1);
      end.set_to_bar (2);
      auto mn = std::make_shared<MidiNote> (r->id_, start, end, 45, 45);
      r->append_object (mn);
      mn->select (true, false, false);
      perform_create (MIDI_SELECTIONS);

      /* undo MIDI note */
      UNDO_MANAGER->undo ();

      /* undo duplicate */
      UNDO_MANAGER->undo ();

      test_project_save_and_reload ();

      /* undo and check that the objects are at
       * their original state*/
      UNDO_MANAGER->undo ();
      check_vs_original_state (false);

      test_project_save_and_reload ();

      /* redo and check that the objects are moved
       * again */
      UNDO_MANAGER->redo ();
      check_after_duplicate_timeline (i, true);

      test_project_save_and_reload ();

      /* undo again to prepare for next test */
      UNDO_MANAGER->undo ();
      check_vs_original_state (false);

      ASSERT_EMPTY (REGION_LINK_GROUP_MANAGER.groups_);
    }
}

TEST_F (ArrangerSelectionsFixture, EditMarker)
{
  /* create marker with name "aa" */
  auto m = std::make_shared<Marker> ("aa");
  m->select (true, false, false);
  ASSERT_NO_THROW (
    m = std::dynamic_pointer_cast<Marker> (m->add_clone_to_project (false)));
  perform_create (TL_SELECTIONS);

  /* change name */
  {
    auto clone_sel = TL_SELECTIONS->clone_unique ();
    auto m2 = clone_sel->get_objects_of_type<Marker> ().front ();
    m2->set_name ("bb", false);
    UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
      *TL_SELECTIONS, clone_sel.get (),
      ArrangerSelectionsAction::EditType::Name, false));
  }

  /* assert name changed */
  ASSERT_EQ (m->name_, "bb");

  UNDO_MANAGER->undo ();
  ASSERT_EQ (m->name_, "aa");

  /* undo again and check that all objects are at
   * original state */
  UNDO_MANAGER->undo ();

  /* redo and check that the name is changed */
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();

  /* return to original state */
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
}

TEST_F (ArrangerSelectionsFixture, MuteObjects)
{
  auto midi_track = TRACKLIST->get_track<MidiTrack> (5);
  ASSERT_NONNULL (midi_track);

  auto &r = midi_track->lanes_[MIDI_REGION_LANE]->regions_[0];

  UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
    *TL_SELECTIONS, nullptr, ArrangerSelectionsAction::EditType::Mute, false));

  auto assert_muted = [&] (bool muted) {
    ASSERT_TRUE (r->is_region ());
    ASSERT_EQ (r->muted_, muted);
    ASSERT_EQ (r->get_muted (false), muted);
  };

  /* assert muted */
  assert_muted (true);

  UNDO_MANAGER->undo ();
  assert_muted (false);

  /* redo and recheck */
  UNDO_MANAGER->redo ();
  assert_muted (true);

  /* return to original state */
  UNDO_MANAGER->undo ();
  assert_muted (false);
}

TEST_F (ArrangerSelectionsFixture, SplitRegion)
{
  ASSERT_SIZE_EQ (P_CHORD_TRACK->regions_, 1);

  Position pos, end_pos;
  pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  auto r = std::make_shared<ChordRegion> (pos, end_pos, 0);
  P_CHORD_TRACK->Track::add_region (r, nullptr, -1, true, false);
  r->select (true, false, false);

  perform_create (TL_SELECTIONS);

  ASSERT_SIZE_EQ (P_CHORD_TRACK->regions_, 2);

  pos.set_to_bar (3);
  perform_split (TL_SELECTIONS, pos);

  ASSERT_SIZE_EQ (P_CHORD_TRACK->regions_, 3);

  auto  track2 = TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
  auto &lane2 = track2->lanes_[3];
  ASSERT_SIZE_EQ (lane2->regions_, 1);

  auto &region2 = lane2->regions_[0];
  region2->validate (false, 0);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();

  /* --- test audio region split --- */

  auto track = TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
  auto lane = track->lanes_.at (3).get ();
  ASSERT_SIZE_EQ (lane->regions_, 1);

  auto &region = lane->regions_[0];
  region->validate (false, 0);
  region->select (true, false, false);
  auto clip = region->get_clip ();
  ASSERT_NONNULL (clip);
  float first_frame = clip->frames_.getSample (0, 0);

  pos.set_to_bar (2);
  pos.add_beats (1);

  ASSERT_NO_THROW (perform_split (TL_SELECTIONS, pos));

  test_project_save_and_reload ();

  /* check that clip frames are the same as before */
  track = TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
  lane = track->lanes_.at (3).get ();
  region = lane->regions_[0];
  clip = region->get_clip ();
  ASSERT_NEAR (first_frame, clip->frames_.getSample (0, 0), 0.000001f);

  UNDO_MANAGER->undo ();
}

TEST_F (ArrangerSelectionsFixture, SplitLargeAudioFile)
{
#ifdef TEST_SINE_OGG_30MIN
  test_project_stop_dummy_engine ();
  auto track = TRACKLIST->append_track (
    *AudioTrack::create_unique (
      "test track", TRACKLIST->tracks_.size (), AUDIO_ENGINE->sample_rate_),
    false, false);
  auto     track_name_hash = track->get_name_hash ();
  Position pos;
  pos.set_to_bar (3);
  auto r = std::make_shared<AudioRegion> (
    -1, TEST_SINE_OGG_30MIN, true, nullptr, 0, std::nullopt, 0, (BitDepth) 0,
    pos, track_name_hash, AUDIO_REGION_LANE, 0);
  auto clip = r->get_clip ();
  ASSERT_EQ (clip->num_frames_, 79380000);
  ASSERT_NO_THROW (track->add_region (r, nullptr, 0, true, false));
  TL_SELECTIONS->add_object_ref (r);

  /* attempt split */
  pos.set_to_bar (4);
  perform_split (TL_SELECTIONS, pos);
#endif
}

TEST_F (ArrangerSelectionsFixture, Quantize)
{
  auto audio_track =
    TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
  ASSERT_NONNULL (audio_track);

  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::QuantizeAction> (
      *AUDIO_SELECTIONS, *QUANTIZE_OPTIONS_EDITOR));

  /* TODO test audio/MIDI quantization */
  auto region = audio_track->lanes_.at (3)->regions_.at (0);
  ASSERT_TRUE (region->is_audio ());

  /* return to original state */
  UNDO_MANAGER->undo ();
}

static void
verify_audio_function (std::vector<float> &frames, size_t max_frames)
{
  auto track = TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
  ASSERT_NONNULL (track);
  auto &lane = track->lanes_.at (3);
  auto &region = lane->regions_.at (0);
  auto  clip = region->get_clip ();
  ASSERT_NONNULL (clip);
  size_t num_frames = std::min (max_frames, (size_t) clip->num_frames_);
  for (size_t i = 0; i < num_frames; i++)
    {
      for (size_t j = 0; j < clip->channels_; j++)
        {
          ASSERT_NEAR (
            frames[clip->channels_ * i + j],
            clip->frames_.getSample (0, clip->channels_ * i + j), 0.0001f);
        }
    }
}

TEST_F (ArrangerSelectionsFixture, AudioFunctions)
{
  auto audio_track =
    TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
  ASSERT_NONNULL (audio_track);
  auto &lane = audio_track->lanes_.at (3);
  ASSERT_SIZE_EQ (lane->regions_, 1);
  auto &region = lane->regions_.at (0);
  region->select (true, false, false);
  AUDIO_SELECTIONS->region_id_ = region->id_;
  AUDIO_SELECTIONS->has_selection_ = true;
  AUDIO_SELECTIONS->sel_start_ = region->pos_;
  AUDIO_SELECTIONS->sel_end_ = region->end_pos_;

  auto               orig_clip = region->get_clip ();
  size_t             channels = orig_clip->channels_;
  auto               frames_per_channel = (size_t) orig_clip->num_frames_;
  size_t             total_frames = (size_t) orig_clip->num_frames_ * channels;
  std::vector<float> orig_frames (total_frames);
  std::vector<float> inverted_frames (total_frames);
  dsp_copy (
    orig_frames.data (), orig_clip->frames_.getReadPointer (0), total_frames);
  dsp_copy (
    inverted_frames.data (), orig_clip->frames_.getReadPointer (0),
    total_frames);
  dsp_mul_k2 (inverted_frames.data (), -1.f, total_frames);

  verify_audio_function (orig_frames, frames_per_channel);

  /* invert */
  AudioFunctionOpts opts = {};
  UNDO_MANAGER->perform (EditArrangerSelectionsAction::create (
    *AUDIO_SELECTIONS, AudioFunctionType::Invert, opts, nullptr));

  verify_audio_function (inverted_frames, frames_per_channel);

  test_project_save_and_reload ();

  verify_audio_function (inverted_frames, frames_per_channel);

  UNDO_MANAGER->undo ();

  verify_audio_function (orig_frames, frames_per_channel);

  UNDO_MANAGER->redo ();

  /* verify that frames are edited again */
  verify_audio_function (inverted_frames, frames_per_channel);

  UNDO_MANAGER->undo ();
}

TEST_F (ArrangerSelectionsFixture, AutomationFill)
{
  /* check automation region */
  auto at = P_MASTER_TRACK->channel_->get_automation_track (
    PortIdentifier::Flags::StereoBalance);
  ASSERT_NONNULL (at);
  ASSERT_SIZE_EQ (at->regions_, 1);

  Position start_pos, end_pos;
  end_pos.set_to_bar (4);
  auto &r1 = at->regions_[0];

  auto r1_clone = r1->clone_shared ();

  auto ap = std::make_shared<AutomationPoint> (0.5f, 0.5f, start_pos);
  r1->append_object (ap);
  ap->select (true, false, false);
  start_pos.add_frames (14);
  ap = std::make_unique<AutomationPoint> (0.6f, 0.6f, start_pos);
  r1->append_object (ap);
  ap->select (true, true, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::AutomationFillAction> (
      *r1_clone, *r1, true));

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
}

TEST_F (ArrangerSelectionsFixture, DuplicateMidiRegionsToTrackBelow)
{
  auto midi_track = TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
  ASSERT_NONNULL (midi_track);
  auto &lane = midi_track->lanes_.at (0);
  ASSERT_EMPTY (lane->regions_);

  Position pos, end_pos;
  pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  auto r1 = std::make_shared<MidiRegion> (
    pos, end_pos, midi_track->get_name_hash (), 0, lane->regions_.size ());
  midi_track->add_region (r1, nullptr, lane->pos_, true, false);
  r1->select (true, false, true);
  perform_create (TL_SELECTIONS);
  ASSERT_SIZE_EQ (lane->regions_, 1);

  pos.set_to_bar (5);
  end_pos.set_to_bar (7);
  auto r2 = std::make_shared<MidiRegion> (
    pos, end_pos, midi_track->get_name_hash (), 0, lane->regions_.size ());
  midi_track->add_region (r2, nullptr, lane->pos_, true, false);
  r2->select (true, false, true);
  perform_create (TL_SELECTIONS);
  ASSERT_SIZE_EQ (lane->regions_, 2);

  /* select the regions */
  r2->select (true, false, true);
  r1->select (true, true, true);

  auto new_midi_track =
    TRACKLIST->find_track_by_name<MidiTrack> (TARGET_MIDI_TRACK_NAME);
  ASSERT_NONNULL (new_midi_track);
  auto &target_lane = new_midi_track->lanes_.at (0);
  ASSERT_SIZE_EQ (target_lane->regions_, 0);

  constexpr auto ticks = 100.0;

  /* replicate the logic from the arranger */
  TL_SELECTIONS->set_index_in_prev_lane ();
  TL_SELECTIONS->move_regions_to_new_tracks (
    new_midi_track->pos_ - midi_track->pos_);
  TL_SELECTIONS->add_ticks (ticks);
  ASSERT_SIZE_EQ (target_lane->regions_, 2);
  ASSERT_SIZE_EQ (TL_SELECTIONS->get_objects_of_type<Region> (), 2);

  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, false, ticks, new_midi_track->pos_ - midi_track->pos_, 0,
      nullptr, true));

  /* check that new regions are created */
  ASSERT_SIZE_EQ (target_lane->regions_, 2);

  UNDO_MANAGER->undo ();

  /* check that new regions are deleted */
  ASSERT_SIZE_EQ (target_lane->regions_, 0);

  UNDO_MANAGER->redo ();

  /* check that new regions are created */
  ASSERT_SIZE_EQ (target_lane->regions_, 2);
}

TEST_F (ArrangerSelectionsFixture, MidiRegionSplit)
{
  auto midi_track = TRACKLIST->find_track_by_name<MidiTrack> (MIDI_TRACK_NAME);
  ASSERT_NONNULL (midi_track);
  auto &lane = midi_track->lanes_.at (0);
  ASSERT_EMPTY (lane->regions_);

  Position pos, end_pos;
  pos.set_to_bar (1);
  end_pos.set_to_bar (5);
  auto r = std::make_shared<MidiRegion> (
    pos, end_pos, midi_track->get_name_hash (), 0, lane->regions_.size ());
  midi_track->add_region (r, nullptr, lane->pos_, true, false);
  r->select (true, false, true);
  perform_create (TL_SELECTIONS);
  ASSERT_SIZE_EQ (lane->regions_, 1);

  /* create some MIDI notes */
  for (int i = 0; i < 4; i++)
    {
      pos.set_to_bar (i + 1);
      end_pos.set_to_bar (i + 2);
      auto mn = std::make_shared<MidiNote> (r->id_, pos, end_pos, 34 + i, 70);
      r->append_object (mn);
      mn->select (true, false, false);
      perform_create (MIDI_SELECTIONS);
      ASSERT_SIZE_EQ (r->midi_notes_, i + 1);
    }

  /* select the region */
  r->select (true, false, false);

  /* split at bar 2 */
  pos.set_to_bar (2);
  perform_split (TL_SELECTIONS, pos);
  ASSERT_SIZE_EQ (lane->regions_, 2);
  r = lane->regions_[1];
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  /* split at bar 4 */
  pos.set_to_bar (4);
  r->select (true, false, false);
  perform_split (TL_SELECTIONS, pos);
  ASSERT_SIZE_EQ (lane->regions_, 3);

  r = lane->regions_.at (0);
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[1];
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[2];
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  /* split at bar 3 */
  r = lane->regions_[1];
  r->select (true, false, false);
  pos.set_to_bar (3);
  r->select (true, false, false);
  perform_split (TL_SELECTIONS, pos);
  ASSERT_SIZE_EQ (lane->regions_, 4);

  r = lane->regions_[0];
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[1];
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[2];
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (3);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[3];
  pos.set_to_bar (3);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  /* undo and verify */
  UNDO_MANAGER->undo ();
  ASSERT_SIZE_EQ (lane->regions_, 3);

  r = lane->regions_[0];
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[1];
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[2];
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  /* undo and verify */
  UNDO_MANAGER->undo ();
  ASSERT_SIZE_EQ (lane->regions_, 2);

  r = lane->regions_[0];
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[1];
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  /* undo and verify */
  UNDO_MANAGER->undo ();
  ASSERT_SIZE_EQ (lane->regions_, 1);

  r = lane->regions_[0];
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  /* redo to bring 3 regions back */
  UNDO_MANAGER->redo ();
  ASSERT_SIZE_EQ (lane->regions_, 2);

  r = lane->regions_[0];
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[1];
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  UNDO_MANAGER->redo ();
  ASSERT_SIZE_EQ (lane->regions_, 3);

  r = lane->regions_[0];
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[1];
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[2];
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  /* delete middle cut */
  r = lane->regions_[1];
  r->select (true, false, false);
  perform_delete (TL_SELECTIONS);

  ASSERT_SIZE_EQ (lane->regions_, 2);

  r = lane->regions_[0];
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[1];
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  /* undo to bring it back */
  UNDO_MANAGER->undo ();
  ASSERT_SIZE_EQ (lane->regions_, 3);

  r = lane->regions_[0];
  pos.set_to_bar (1);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[1];
  pos.set_to_bar (2);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);

  r = lane->regions_[2];
  pos.set_to_bar (4);
  ASSERT_EQ (pos.frames_, r->pos_.frames_);
  pos.set_to_bar (5);
  ASSERT_EQ (pos.frames_, r->end_pos_.frames_);
}

TEST_F (ArrangerSelectionsFixture, PinUnpin)
{
  auto &r = P_CHORD_TRACK->regions_.at (0);
  P_CHORD_TRACK->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<UnpinTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR));

  REQUIRE_OBJ_TRACK_NAME_HASH_MATCHES_TRACK (r, *P_CHORD_TRACK);

  /* TODO more tests */
}

TEST_F (ArrangerSelectionsFixture, DeleteMarkers)
{
  /* create markers A B C D */
  const char *            names[4] = { "A", "B", "C", "D" };
  std::shared_ptr<Marker> m_c, m_d;
  for (int i = 0; i < 4; i++)
    {
      auto m = std::make_shared<Marker> (names[i]);
      P_MARKER_TRACK->add_marker (m);
      m->select (true, false, false);
      perform_create (TL_SELECTIONS);

      if (i == 2)
        {
          m_c = m;
        }
      else if (i == 3)
        {
          m_d = m;
        }
    }

  /* delete C */
  m_c->select (true, false, false);
  perform_delete (TL_SELECTIONS);

  /* delete D */
  m_d->select (true, false, false);
  perform_delete (TL_SELECTIONS);

  for (int i = 0; i < 6; i++)
    {
      UNDO_MANAGER->undo ();
    }
}

TEST_F (ArrangerSelectionsFixture, DeleteScaleObjects)
{
  /* create markers A B C D */
  std::shared_ptr<ScaleObject> m_c, m_d;
  for (int i = 0; i < 4; i++)
    {
      MusicalScale ms (
        ENUM_INT_TO_VALUE (MusicalScale::Type, i),
        ENUM_INT_TO_VALUE (MusicalNote, i));
      auto m = std::make_shared<ScaleObject> (ms);
      P_CHORD_TRACK->add_scale (m);
      m->select (true, false, false);
      perform_create (TL_SELECTIONS);

      if (i == 2)
        {
          m_c = m;
        }
      else if (i == 3)
        {
          m_d = m;
        }
    }

  /* delete C */
  m_c->select (true, false, false);
  perform_delete (TL_SELECTIONS);

  /* delete D */
  m_d->select (true, false, false);
  perform_delete (TL_SELECTIONS);

  for (int i = 0; i < 6; i++)
    {
      UNDO_MANAGER->undo ();
    }
}

TEST_F (ArrangerSelectionsFixture, DeleteChordObjects)
{
  Position pos1, pos2;
  pos1.set_to_bar (1);
  pos2.set_to_bar (4);
  auto r = std::make_shared<ChordRegion> (pos1, pos2, 0);
  P_CHORD_TRACK->Track::add_region (r, nullptr, 0, true, 0);
  TL_SELECTIONS->add_object_ref (r);
  perform_create (TL_SELECTIONS);

  /* create markers A B C D */
  std::shared_ptr<ChordObject> m_c, m_d;
  for (int i = 0; i < 4; i++)
    {
      auto m = std::make_shared<ChordObject> (r->id_, i, i);
      r->append_object (m);
      m->select (true, false, false);
      perform_create (CHORD_SELECTIONS);

      if (i == 2)
        {
          m_c = m;
        }
      else if (i == 3)
        {
          m_d = m;
        }
    }

  /* delete C */
  m_c->select (true, false, false);
  perform_delete (CHORD_SELECTIONS);

  /* delete D */
  m_d->select (true, false, false);
  perform_delete (CHORD_SELECTIONS);

  for (int i = 0; i < 6; i++)
    {
      UNDO_MANAGER->undo ();
    }
}

TEST_F (ArrangerSelectionsFixture, DeleteAutomationPoints)
{
  Position pos1, pos2;
  pos1.set_to_bar (1);
  pos2.set_to_bar (4);
  auto at = P_MASTER_TRACK->channel_->get_automation_track (
    PortIdentifier::Flags::ChannelFader);
  ASSERT_NONNULL (at);
  auto r = std::make_shared<AutomationRegion> (
    pos1, pos2, P_MASTER_TRACK->get_name_hash (), at->index_, 0);
  P_MASTER_TRACK->add_region (r, at, 0, true, false);
  TL_SELECTIONS->add_object_ref (r);
  perform_create (TL_SELECTIONS);

  /* create markers A B C D */
  std::shared_ptr<AutomationPoint> m_c, m_d;
  for (int i = 0; i < 4; i++)
    {
      auto m = std::make_shared<AutomationPoint> (1.f, 1.f, pos1);
      r->append_object (m);
      m->select (true, false, false);
      perform_create (AUTOMATION_SELECTIONS);

      if (i == 2)
        {
          m_c = m;
        }
      else if (i == 3)
        {
          m_d = m;
        }
    }

  /* delete C */
  m_c->select (true, false, false);
  perform_delete (AUTOMATION_SELECTIONS);

  /* delete D */
  m_d->select (true, false, false);
  perform_delete (AUTOMATION_SELECTIONS);

  for (int i = 0; i < 6; i++)
    {
      UNDO_MANAGER->undo ();
    }
}

TEST_F (ZrythmFixture, DuplicateAudioRegions)
{
  auto audio_file_path = fs::path (TESTS_SRCDIR) / "test.wav";

  /* create audio track with region */
  Position       pos1;
  int            track_pos = TRACKLIST->tracks_.size ();
  FileDescriptor file (audio_file_path);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos1, track_pos, 1, -1, nullptr);
  auto track = TRACKLIST->get_track<AudioTrack> (track_pos);

  track->lanes_[0]->regions_[0]->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, false, MOVE_TICKS, 0, 0, nullptr, false));

  test_project_save_and_reload ();
}

TEST_F (ZrythmFixture, UndoMovingMidiRegionToOtherLane)
{
  /* create midi track with region */
  Track::create_empty_with_action (Track::Type::Midi);
  auto midi_track = dynamic_cast<MidiTrack *> (
    TRACKLIST->get_last_track (Tracklist::PinOption::Both, false));
  ASSERT_NONNULL (midi_track);
  ASSERT_EQ (midi_track->type_, Track::Type::Midi);

  std::shared_ptr<MidiRegion> r;
  for (int i = 0; i < 4; i++)
    {
      Position start, end;
      end.add_bars (1);
      int lane_pos = (i == 3) ? 2 : 0;
      int idx_inside_lane = (i == 3) ? 0 : i;
      r = std::make_shared<MidiRegion> (
        start, end, midi_track->get_name_hash (), lane_pos, idx_inside_lane);
      midi_track->add_region (r, nullptr, lane_pos, true, false);
      r->select (true, false, false);
      perform_create (TL_SELECTIONS);
    }

  /* move last region to top lane */
  r->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, true, MOVE_TICKS, 0, -2, nullptr, false));

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();
}

TEST_F (ZrythmFixture, DeleteMultipleRegions)
{
  auto midi_track = dynamic_cast<MidiTrack *> (
    Track::create_empty_with_action (Track::Type::Midi));
  ASSERT_NONNULL (midi_track);

  auto &lane = midi_track->lanes_[0];
  for (int i = 0; i < 6; i++)
    {
      Position pos, end_pos;
      pos.set_to_bar (2 + i);
      end_pos.set_to_bar (4 + i);
      auto r1 = std::make_shared<MidiRegion> (
        pos, end_pos, midi_track->get_name_hash (), 0, lane->regions_.size ());
      midi_track->add_region (r1, nullptr, lane->pos_, true, false);
      r1->select (true, false, false);
      perform_create (TL_SELECTIONS);
      ASSERT_SIZE_EQ (lane->regions_, i + 1);
    }

  /* select multiple and delete */
  for (int i = 0; i < 100; i++)
    {
      int idx1 = rand () % 6;
      int idx2 = rand () % 6;
      lane->regions_[idx1]->select (true, false, false);
      lane->regions_[idx2]->select (true, true, false);

      /* do delete */
      ASSERT_NO_THROW (perform_delete (TL_SELECTIONS));

      CLIP_EDITOR->get_region ();

      ASSERT_NO_THROW (UNDO_MANAGER->undo ());

      ASSERT_NONNULL (CLIP_EDITOR->get_region ());
    }

  ASSERT_NONNULL (CLIP_EDITOR->get_region ());
}

TEST_F (ZrythmFixture, SplitAndMergeMidiUnlooped)
{
  Position pos, end_pos, tmp;

  auto midi_track = dynamic_cast<MidiTrack *> (
    Track::create_empty_with_action (Track::Type::Midi));
  ASSERT_NONNULL (midi_track);

  auto &lane = midi_track->lanes_[0];
  pos.set_to_bar (2);
  end_pos.set_to_bar (10);
  auto r1 = std::make_shared<MidiRegion> (
    pos, end_pos, midi_track->get_name_hash (), 0, lane->regions_.size ());
  midi_track->add_region (r1, nullptr, lane->pos_, true, false);
  r1->select (true, false, false);
  perform_create (TL_SELECTIONS);
  ASSERT_SIZE_EQ (lane->regions_, 1);

  for (int i = 0; i < 2; i++)
    {
      if (i == 0)
        {
          pos.set_to_bar (1);
          end_pos.set_to_bar (2);
        }
      else
        {
          pos.set_to_bar (5);
          end_pos.set_to_bar (6);
        }
      auto mn = std::make_shared<MidiNote> (r1->id_, pos, end_pos, 45, 45);
      r1->append_object (mn);
      mn->select (true, false, false);
      perform_create (MIDI_SELECTIONS);
    }

  /* split */
  auto r = lane->regions_[0];
  tmp.set_to_bar (2);
  ASSERT_POSITION_EQ (r->pos_, tmp);
  r->select (true, false, false);
  Position split_pos;
  split_pos.set_to_bar (4);
  ASSERT_NO_THROW (perform_split (TL_SELECTIONS, split_pos));

  /* check r1 positions */
  r1 = lane->regions_[0];
  tmp.set_to_bar (2);
  ASSERT_POSITION_EQ (r1->pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r1->clip_start_pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r1->loop_start_pos_, tmp);
  tmp.set_to_bar (4);
  ASSERT_POSITION_EQ (r1->end_pos_, tmp);
  tmp.set_to_bar (3);
  ASSERT_POSITION_EQ (r1->loop_end_pos_, tmp);

  /* check r1 midi note positions */
  ASSERT_SIZE_EQ (r1->midi_notes_, 1);
  auto mn = r1->midi_notes_.at (0);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (mn->pos_, tmp);
  tmp.set_to_bar (2);
  ASSERT_POSITION_EQ (mn->end_pos_, tmp);

  /* check r2 positions */
  auto r2 = lane->regions_[1];
  tmp.set_to_bar (4);
  ASSERT_POSITION_EQ (r2->pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r2->clip_start_pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r2->loop_start_pos_, tmp);
  tmp.set_to_bar (10);
  ASSERT_POSITION_EQ (r2->end_pos_, tmp);
  tmp.set_to_bar (7);
  ASSERT_POSITION_EQ (r2->loop_end_pos_, tmp);

  /* check r2 midi note positions */
  ASSERT_SIZE_EQ (r2->midi_notes_, 1);
  mn = r2->midi_notes_[0];
  tmp.set_to_bar (3);
  ASSERT_POSITION_EQ (mn->pos_, tmp);
  tmp.set_to_bar (4);
  ASSERT_POSITION_EQ (mn->end_pos_, tmp);

  /* merge */
  lane->regions_[0]->select (true, false, false);
  lane->regions_[1]->select (true, true, false);
  ASSERT_NO_THROW (UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MergeAction> (*TL_SELECTIONS)));

  /* verify positions */
  ASSERT_SIZE_EQ (lane->regions_, 1);
  r = lane->regions_[0];
  tmp.set_to_bar (2);
  ASSERT_POSITION_EQ (r->pos_, tmp);
  tmp.set_to_bar (10);
  ASSERT_POSITION_EQ (r->end_pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r->loop_start_pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r->clip_start_pos_, tmp);
  tmp.set_to_bar (9);
  ASSERT_POSITION_EQ (r->loop_end_pos_, tmp);

  CLIP_EDITOR->get_region ();

  ASSERT_NO_THROW (UNDO_MANAGER->undo ());

  /* check r1 positions */
  r1 = lane->regions_[0];
  tmp.set_to_bar (2);
  ASSERT_POSITION_EQ (r1->pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r1->clip_start_pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r1->loop_start_pos_, tmp);
  tmp.set_to_bar (4);
  ASSERT_POSITION_EQ (r1->end_pos_, tmp);
  tmp.set_to_bar (3);
  ASSERT_POSITION_EQ (r1->loop_end_pos_, tmp);

  /* check r1 midi note positions */
  ASSERT_SIZE_EQ (r1->midi_notes_, 1);
  mn = r1->midi_notes_[0];
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (mn->pos_, tmp);
  tmp.set_to_bar (2);
  ASSERT_POSITION_EQ (mn->end_pos_, tmp);

  /* check r2 positions */
  r2 = lane->regions_[1];
  tmp.set_to_bar (4);
  ASSERT_POSITION_EQ (r2->pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r2->clip_start_pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r2->loop_start_pos_, tmp);
  tmp.set_to_bar (10);
  ASSERT_POSITION_EQ (r2->end_pos_, tmp);
  tmp.set_to_bar (7);
  ASSERT_POSITION_EQ (r2->loop_end_pos_, tmp);

  /* check r2 midi note positions */
  ASSERT_SIZE_EQ (r2->midi_notes_, 1);
  mn = r2->midi_notes_[0];
  tmp.set_to_bar (3);
  ASSERT_POSITION_EQ (mn->pos_, tmp);
  tmp.set_to_bar (4);
  ASSERT_POSITION_EQ (mn->end_pos_, tmp);

  /* undo split */
  ASSERT_NO_THROW (UNDO_MANAGER->undo ());

  /* verify region */
  r = lane->regions_[0];
  tmp.set_to_bar (2);
  ASSERT_POSITION_EQ (r->pos_, tmp);
  tmp.set_to_bar (10);
  ASSERT_POSITION_EQ (r->end_pos_, tmp);
  tmp.set_to_bar (9);
  ASSERT_POSITION_EQ (r->loop_end_pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r->clip_start_pos_, tmp);
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (r->loop_start_pos_, tmp);

  /* verify midi notes are back to start */
  ASSERT_SIZE_EQ (r->midi_notes_, 2);
  mn = r->midi_notes_[0];
  tmp.set_to_bar (1);
  ASSERT_POSITION_EQ (mn->pos_, tmp);
  tmp.set_to_bar (2);
  ASSERT_POSITION_EQ (mn->end_pos_, tmp);
  mn = r->midi_notes_[1];
  tmp.set_to_bar (5);
  ASSERT_POSITION_EQ (mn->pos_, tmp);
  tmp.set_to_bar (6);
  ASSERT_POSITION_EQ (mn->end_pos_, tmp);

  ASSERT_NONNULL (CLIP_EDITOR->get_region ());

  ASSERT_NO_THROW (
    UNDO_MANAGER->redo (); UNDO_MANAGER->redo (); UNDO_MANAGER->undo ();
    UNDO_MANAGER->undo ());
}

TEST_F (ZrythmFixture, SplitAndMergeAudioUnlooped)
{
  Position pos, tmp;

  auto           audio_file_path = fs::path (TESTS_SRCDIR) / "test.wav";
  FileDescriptor file_descr (audio_file_path);
  pos.set_to_bar (2);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file_descr, &pos, TRACKLIST->tracks_.size (),
    1, -1, nullptr);
  auto audio_track = dynamic_cast<AudioTrack *> (
    TRACKLIST->get_last_track (Tracklist::PinOption::Both, false));
  ASSERT_NONNULL (audio_track);
  int audio_track_pos = audio_track->pos_;
  ASSERT_SIZE_EQ (audio_track->lanes_, 2);
  ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 1);
  ASSERT_SIZE_EQ (audio_track->lanes_[1]->regions_, 0);

  const auto frames_per_bar =
    (unsigned_frame_t) (AUDIO_ENGINE->frames_per_tick_
                        * (double) TRANSPORT->ticks_per_bar_);

  std::vector<float> l_frames;

  {
    /* <2.1.1.0> to around <4.1.1.0> (around 2 bars long) */
    auto &lane = audio_track->lanes_[0];
    auto &r = lane->regions_[0];
    ASSERT_POSITION_EQ (r->pos_, pos);

    /* remember frames */
    auto clip = r->get_clip ();
    ASSERT_GT (clip->num_frames_, 0);
    l_frames.resize (clip->num_frames_, 0);
    dsp_copy (
      l_frames.data (), clip->ch_frames_.getReadPointer (0),
      (size_t) clip->num_frames_);

    /* split */
    r = lane->regions_[0];
    tmp.set_to_bar (2);
    ASSERT_POSITION_EQ (r->pos_, tmp);
    r->select (true, false, false);
    Position split_pos;
    split_pos.set_to_bar (3);
    perform_split (TL_SELECTIONS, split_pos);

    /* check r1 positions */
    auto &r1 = lane->regions_[0];
    tmp.set_to_bar (2);
    ASSERT_POSITION_EQ (r1->pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r1->clip_start_pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r1->loop_start_pos_, tmp);
    tmp.set_to_bar (3);
    ASSERT_POSITION_EQ (r1->end_pos_, tmp);
    tmp.set_to_bar (2);
    ASSERT_POSITION_EQ (r1->loop_end_pos_, tmp);

    /* check r1 audio positions */
    auto r1_clip = r1->get_clip ();
    ASSERT_EQ (r1_clip->num_frames_, frames_per_bar);
    ASSERT_TRUE (audio_frames_equal (
      r1_clip->ch_frames_.getReadPointer (0), &l_frames[0],
      (size_t) r1_clip->num_frames_, 0.0001f));

    /* check r2 positions */
    auto &r2 = lane->regions_[1];
    tmp.set_to_bar (3);
    ASSERT_POSITION_EQ (r2->pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r2->clip_start_pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r2->loop_start_pos_, tmp);
    ASSERT_EQ (
      (unsigned_frame_t) r2->end_pos_.frames_,
      /* total previous frames + started at bar 2 (1 bar) */
      l_frames.size () + frames_per_bar);
    ASSERT_EQ (
      (unsigned_frame_t) r2->loop_end_pos_.frames_,
      /* total previous frames - r1 frames */
      l_frames.size () - r1_clip->num_frames_);

    /* check r2 audio positions */
    auto r2_clip = r2->get_clip ();
    ASSERT_EQ (
      r2_clip->num_frames_, (unsigned_frame_t) r2->loop_end_pos_.frames_);
    ASSERT_TRUE (audio_frames_equal (
      r2_clip->ch_frames_.getReadPointer (0), &l_frames[frames_per_bar],
      (size_t) r2_clip->num_frames_, 0.0001f));

    /* merge */
    lane->regions_[0]->select (true, false, false);
    lane->regions_[1]->select (true, true, false);
    UNDO_MANAGER->perform (
      std::make_unique<ArrangerSelectionsAction::MergeAction> (*TL_SELECTIONS));

    /* verify positions */
    ASSERT_SIZE_EQ (lane->regions_, 1);
    r = lane->regions_[0];
    tmp.set_to_bar (2);
    ASSERT_POSITION_EQ (r->pos_, tmp);
    tmp.set_to_bar (2);
    tmp.add_frames ((signed_frame_t) l_frames.size ());
    ASSERT_POSITION_EQ (r->end_pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r->loop_start_pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r->clip_start_pos_, tmp);
    tmp.from_frames ((signed_frame_t) l_frames.size ());
    ASSERT_POSITION_EQ (r->loop_end_pos_, tmp);

    CLIP_EDITOR->get_region ();
  }

  {
    test_project_save_and_reload ();
    audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
    ASSERT_NONNULL (audio_track);
    auto &lane = audio_track->lanes_[0];
    ASSERT_NONNULL (lane);
  }

  {
    /* undo merge */
    UNDO_MANAGER->undo ();
    test_project_save_and_reload ();
    audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
    ASSERT_NONNULL (audio_track);
    auto &lane = audio_track->lanes_[0];

    /* check r1 positions */
    auto &r1 = lane->regions_[0];
    tmp.set_to_bar (2);
    ASSERT_POSITION_EQ (r1->pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r1->clip_start_pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r1->loop_start_pos_, tmp);
    tmp.set_to_bar (3);
    ASSERT_POSITION_EQ (r1->end_pos_, tmp);
    tmp.set_to_bar (2);
    ASSERT_POSITION_EQ (r1->loop_end_pos_, tmp);

    /* check r1 audio positions */
    auto r1_clip = r1->get_clip ();
    ASSERT_EQ (r1_clip->num_frames_, frames_per_bar);
    ASSERT_TRUE (audio_frames_equal (
      r1_clip->ch_frames_.getReadPointer (0), &l_frames[0],
      (size_t) r1_clip->num_frames_, 0.0001f));

    /* check r2 positions */
    auto &r2 = lane->regions_[1];
    tmp.set_to_bar (3);
    ASSERT_POSITION_EQ (r2->pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r2->clip_start_pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r2->loop_start_pos_, tmp);
    ASSERT_EQ (
      (unsigned_frame_t) r2->end_pos_.frames_,
      /* total previous frames + started at bar 2 (1 bar) */
      l_frames.size () + frames_per_bar);
    ASSERT_EQ (
      (unsigned_frame_t) r2->loop_end_pos_.frames_,
      /* total previous frames - r1 frames */
      l_frames.size () - r1_clip->num_frames_);

    /* check r2 audio positions */
    auto r2_clip = r2->get_clip ();
    ASSERT_EQ ((signed_frame_t) r2_clip->num_frames_, r2->loop_end_pos_.frames_);
    ASSERT_TRUE (audio_frames_equal (
      r2_clip->ch_frames_.getReadPointer (0), &l_frames[frames_per_bar],
      (size_t) r2_clip->num_frames_, 0.0001f));
  }

  {
    /* undo split */
    UNDO_MANAGER->undo ();
    test_project_save_and_reload ();
    audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
    auto &lane = audio_track->lanes_[0];

    /* verify region */
    auto &r = lane->regions_[0];
    tmp.set_to_bar (2);
    ASSERT_POSITION_EQ (r->pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r->clip_start_pos_, tmp);
    tmp.set_to_bar (1);
    ASSERT_POSITION_EQ (r->loop_start_pos_, tmp);
    ASSERT_EQ (
      (unsigned_frame_t) r->end_pos_.frames_,
      /* total previous frames + started at bar 2 (1 bar) */
      l_frames.size () + frames_per_bar);
    ASSERT_EQ (
      r->loop_end_pos_.frames_,
      /* total previous frames */
      (signed_frame_t) l_frames.size ());

    /* check frames */
    auto clip = r->get_clip ();
    ASSERT_EQ (clip->num_frames_, l_frames.size ());
    ASSERT_TRUE (audio_frames_equal (
      clip->ch_frames_.getReadPointer (0), l_frames.data (), l_frames.size (),
      0.0001f));
  }

  ASSERT_NONNULL (CLIP_EDITOR->get_region ());

  test_project_save_and_reload ();

  /* redo split */
  UNDO_MANAGER->redo ();
  test_project_save_and_reload ();

  /* redo merge */
  UNDO_MANAGER->redo ();
  test_project_save_and_reload ();

  /* undo merge */
  UNDO_MANAGER->undo ();
  test_project_save_and_reload ();

  /* undo split */
  UNDO_MANAGER->undo ();
  test_project_save_and_reload ();
}

TEST_F (ZrythmFixture, ResizeLoopFromLeftSide)
{
  Position pos, tmp;

  const auto     audio_file_path = fs::path (TESTS_SRCDIR) / "test.wav";
  FileDescriptor file_descr (audio_file_path);
  pos.set_to_bar (3);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file_descr, &pos, TRACKLIST->tracks_.size (),
    1, -1, nullptr);
  auto audio_track = dynamic_cast<AudioTrack *> (
    TRACKLIST->get_last_track (Tracklist::PinOption::Both, false));
  ASSERT_NONNULL (audio_track);
  const auto audio_track_pos = audio_track->pos_;
  ASSERT_SIZE_EQ (audio_track->lanes_, 2);
  ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 1);
  ASSERT_SIZE_EQ (audio_track->lanes_[1]->regions_, 0);

  /* <3.1.1.0> to around <5.1.1.0> (around 2 bars long) */
  auto &lane = audio_track->lanes_[0];
  auto &r = lane->regions_[0];
  ASSERT_POSITION_EQ (r->pos_, pos);

  /* remember end pos */
  Position end_pos = r->end_pos_;

  /* remember loop length */
  double loop_len_ticks = r->get_loop_length_in_ticks ();

  /* resize L */
  r->select (true, false, false);
  const double move_ticks = 100.0;
  UNDO_MANAGER->perform (std::make_unique<ArrangerSelectionsAction::ResizeAction> (
    *TL_SELECTIONS, nullptr, ArrangerSelectionsAction::ResizeType::LLoop,
    -move_ticks));

  r = lane->regions_[0];

  /* test start pos */
  Position new_pos = pos;
  new_pos.add_ticks (-move_ticks);
  ASSERT_POSITION_EQ (r->pos_, new_pos);

  /* test loop start pos */
  new_pos.zero ();
  ASSERT_POSITION_EQ (r->loop_start_pos_, new_pos);

  /* test end pos */
  ASSERT_POSITION_EQ (r->end_pos_, end_pos);

  ASSERT_NEAR (loop_len_ticks, r->get_loop_length_in_ticks (), 0.0001);

  (void) audio_track_pos;
  (void) tmp;
}

TEST_F (ZrythmFixture, DeleteMidiNotes)
{
  auto midi_track = Track::create_empty_with_action<MidiTrack> ();

  /* create region */
  Position start, end;
  start.set_to_bar (1);
  end.set_to_bar (6);
  auto r = std::make_shared<MidiRegion> (
    start, end, midi_track->get_name_hash (), 0, 0);
  midi_track->add_region (r, nullptr, 0, true, false);
  r->select (true, false, false);
  perform_create (TL_SELECTIONS);

  /* create 4 MIDI notes */
  for (int i = 0; i < 4; i++)
    {
      start.set_to_bar (1 + i);
      end.set_to_bar (2 + i);
      auto mn = std::make_shared<MidiNote> (r->id_, start, end, 45, 45);
      r->append_object (mn);
      mn->select (true, false, false);
      perform_create (MIDI_SELECTIONS);
    }

  auto check_indices = [&] () {
    for (size_t i = 0; i < r->midi_notes_.size (); ++i)
      {
        auto &mn = r->midi_notes_[i];
        ASSERT_EQ (mn->index_, i);
      }
  };

  check_indices ();

  for (int j = 0; j < 20; j++)
    {
      /* delete random MIDI notes */
      {
        int   idx = rand () % r->midi_notes_.size ();
        auto &mn = r->midi_notes_[idx];
        mn->select (true, false, false);
        idx = rand () % r->midi_notes_.size ();
        mn = r->midi_notes_[idx];
        mn->select (true, true, false);
        perform_create (MIDI_SELECTIONS);
      }

      check_indices ();

      /* undo */
      UNDO_MANAGER->undo ();

      check_indices ();

      /* redo delete midi note */
      UNDO_MANAGER->redo ();

      check_indices ();

      UNDO_MANAGER->undo ();
    }

#undef CHECK_INDICES
}

TEST_F (ZrythmFixture, CutAutomationRegion)
{
  /* create master fader automation region */
  Position pos1, pos2;
  pos1.set_to_bar (1);
  pos2.set_to_bar (8);
  auto at = P_MASTER_TRACK->channel_->get_automation_track (
    PortIdentifier::Flags::ChannelFader);
  ASSERT_NONNULL (at);
  auto r = std::make_shared<AutomationRegion> (
    pos1, pos2, P_MASTER_TRACK->get_name_hash (), at->index_, 0);
  P_MASTER_TRACK->add_region (r, at, 0, true, false);
  TL_SELECTIONS->add_object_ref (r);
  perform_create (TL_SELECTIONS);

  /* create 2 points spanning the split point */
  for (int i = 0; i < 2; i++)
    {
      pos1.set_to_bar (i == 0 ? 3 : 5);
      auto ap = std::make_shared<AutomationPoint> (1.f, 1.f, pos1);
      r->append_object (ap, false);
      ap->select (true, false, false);
      perform_create (AUTOMATION_SELECTIONS);
    }

  /* split between the 2 points */
  r->select (true, false, false);
  pos1.set_to_bar (4);
  perform_split (TL_SELECTIONS, pos1);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();

  /* split before the first point */
  r = at->regions_[0];
  r->select (true, false, false);
  pos1.set_to_bar (2);
  perform_split (TL_SELECTIONS, pos1);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();
}

TEST_F (ZrythmFixture, CopyAndMoveAutomationRegions)
{
  /* create a new track */
  auto audio_track = Track::create_empty_with_action<AudioTrack> ();

  Position pos1, pos2;
  pos1.set_to_bar (2);
  pos2.set_to_bar (4);
  auto fader_at = P_MASTER_TRACK->channel_->get_automation_track (
    PortIdentifier::Flags::ChannelFader);
  ASSERT_NONNULL (fader_at);
  auto r = std::make_shared<AutomationRegion> (
    pos1, pos2, P_MASTER_TRACK->get_name_hash (), fader_at->index_, 0);
  P_MASTER_TRACK->add_region (r, fader_at, 0, true, false);
  TL_SELECTIONS->add_object_ref (r);
  perform_create (TL_SELECTIONS);

  auto ap = std::make_shared<AutomationPoint> (0.5f, 0.5f, pos1);
  r->append_object (ap);
  ap->select (true, false, false);
  perform_create (AUTOMATION_SELECTIONS);
  ap = std::make_shared<AutomationPoint> (0.6f, 0.6f, pos2);
  r->append_object (ap);
  ap->select (true, false, false);
  perform_create (AUTOMATION_SELECTIONS);

  auto mute_at = audio_track->channel_->get_automation_track (
    PortIdentifier::Flags::FaderMute);
  ASSERT_NONNULL (mute_at);

  /* 1st test */

  /* when i == 1 we are copying */
  for (int i = 0; i < 2; i++)
    {
      bool copy = i == 1;

      ASSERT_SIZE_EQ (fader_at->regions_, 1);
      ASSERT_SIZE_EQ (mute_at->regions_, 0);

      if (copy)
        {
          UNDO_MANAGER->perform (
            std::make_unique<
              ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
              *TL_SELECTIONS, false, 0, 0, 0, &mute_at->port_id_, false));
          ASSERT_SIZE_EQ (fader_at->regions_, 1);
          ASSERT_SIZE_EQ (mute_at->regions_, 1);
          UNDO_MANAGER->undo ();
          ASSERT_SIZE_EQ (fader_at->regions_, 1);
          ASSERT_SIZE_EQ (mute_at->regions_, 0);
          UNDO_MANAGER->redo ();
          ASSERT_SIZE_EQ (fader_at->regions_, 1);
          ASSERT_SIZE_EQ (mute_at->regions_, 1);
          UNDO_MANAGER->undo ();
        }
      else
        {
          UNDO_MANAGER->perform (
            std::make_unique<
              ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
              *TL_SELECTIONS, true, 0, 0, 0, &mute_at->port_id_, false));
          ASSERT_SIZE_EQ (fader_at->regions_, 0);
          ASSERT_SIZE_EQ (mute_at->regions_, 1);
          UNDO_MANAGER->undo ();
          ASSERT_SIZE_EQ (fader_at->regions_, 1);
          ASSERT_SIZE_EQ (mute_at->regions_, 0);
          UNDO_MANAGER->redo ();
          ASSERT_SIZE_EQ (fader_at->regions_, 0);
          ASSERT_SIZE_EQ (mute_at->regions_, 1);
          UNDO_MANAGER->undo ();
        }

      ASSERT_SIZE_EQ (fader_at->regions_, 1);
      ASSERT_SIZE_EQ (mute_at->regions_, 0);
    }

  /* 2nd test */

  /* create 2 copies in the empty lane a bit behind */
  fader_at->regions_[0]->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, false, -200, 0, 0, &mute_at->port_id_, false));
  ASSERT_SIZE_EQ (fader_at->regions_, 1);
  ASSERT_SIZE_EQ (mute_at->regions_, 1);
  fader_at->regions_[0]->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, false, -400, 0, 0, &mute_at->port_id_, false));
  ASSERT_SIZE_EQ (fader_at->regions_, 1);
  ASSERT_SIZE_EQ (mute_at->regions_, 2);

  /* move the copy to the first lane */
  mute_at->regions_[0]->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, true, 0, 0, 0, &mute_at->port_id_, false));
  ASSERT_SIZE_EQ (fader_at->regions_, 2);
  ASSERT_SIZE_EQ (mute_at->regions_, 1);

  /* undo and verify all ok */
  UNDO_MANAGER->undo ();
  ASSERT_SIZE_EQ (fader_at->regions_, 1);
  ASSERT_SIZE_EQ (mute_at->regions_, 2);
  UNDO_MANAGER->redo ();
  ASSERT_SIZE_EQ (fader_at->regions_, 2);
  ASSERT_SIZE_EQ (mute_at->regions_, 1);
  UNDO_MANAGER->undo ();
  ASSERT_SIZE_EQ (fader_at->regions_, 1);
  ASSERT_SIZE_EQ (mute_at->regions_, 2);
}

TEST_F (ZrythmFixture, MoveRegionFromLane3ToLane1)
{
  Position pos, end_pos;
  Track::create_with_action (
    Track::Type::Midi, nullptr, nullptr, &pos, TRACKLIST->tracks_.size (), 1,
    -1, nullptr);
  auto track = dynamic_cast<MidiTrack *> (
    TRACKLIST->get_last_track (Tracklist::PinOption::Both, false));

  auto &orig_lane = track->lanes_[0];
  ASSERT_EMPTY (orig_lane->regions_);

  /* create region */
  pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  auto r1 = std::make_shared<MidiRegion> (
    pos, end_pos, track->get_name_hash (), 0, orig_lane->regions_.size ());
  track->add_region (r1, nullptr, orig_lane->pos_, true, false);
  r1->select (true, false, true);
  perform_create (TL_SELECTIONS);
  ASSERT_SIZE_EQ (orig_lane->regions_, 1);

  {
    /* move to lane 3 */
    UNDO_MANAGER->perform (
      std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
        *TL_SELECTIONS, true, 0, 0, 2, nullptr, false));
    ASSERT_SIZE_EQ (orig_lane->regions_, 0);
    auto &lane = track->lanes_[2];
    ASSERT_SIZE_EQ (lane->regions_, 1);

    /* duplicate track */
    UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
      *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
      track->pos_ + 1));
  }

  {
    /* move new region to lane 1 */
    track = dynamic_cast<MidiTrack *> (
      TRACKLIST->get_last_track (Tracklist::PinOption::Both, true));
    auto &lane = track->lanes_[2];
    r1 = lane->regions_[0];
    r1->select (true, false, true);
  }

  {
    UNDO_MANAGER->perform (
      std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
        *TL_SELECTIONS, true, 0, 0, -2, nullptr, false));
    auto &lane = track->lanes_[0];
    ASSERT_SIZE_EQ (lane->regions_, 1);
  }
}

TEST_F (ArrangerSelectionsFixture, Stretch)
{
  std::vector<float> orig_frames;
  size_t             total_orig_frames;
  {
    auto  track = TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
    auto &lane = track->lanes_[3];
    ASSERT_SIZE_EQ (lane->regions_, 1);

    auto &region = lane->regions_[0];
    region->select (true, false, false);

    auto orig_clip = region->get_clip ();
    total_orig_frames = (size_t) orig_clip->num_frames_ * orig_clip->channels_;
    orig_frames.resize (total_orig_frames, 0.f);
    dsp_copy (
      &orig_frames[0], orig_clip->frames_.getReadPointer (0), total_orig_frames);

    constexpr double move_ticks = 100.0;
    UNDO_MANAGER->perform (
      std::make_unique<ArrangerSelectionsAction::ResizeAction> (
        *TL_SELECTIONS, nullptr, ArrangerSelectionsAction::ResizeType::RStretch,
        move_ticks));
  }

  UNDO_MANAGER->undo ();

  {
    /* test that the original audio is back */
    auto  track = TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
    auto &lane = track->lanes_[3];
    auto &region = lane->regions_[0];
    auto  after_clip = region->get_clip ();
    const size_t total_after_frames =
      (size_t) after_clip->num_frames_ * after_clip->channels_;
    ASSERT_EQ (total_after_frames, total_orig_frames);
    ASSERT_TRUE (audio_frames_equal (
      after_clip->frames_.getReadPointer (0), &orig_frames[0],
      total_orig_frames, 0.0001f));
  }

  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();

  {
    /* test that the original audio is back */
    auto  track = TRACKLIST->find_track_by_name<AudioTrack> (AUDIO_TRACK_NAME);
    auto &lane = track->lanes_[3];
    auto &region = lane->regions_[0];
    auto  after_clip = region->get_clip ();
    const size_t total_after_frames =
      (size_t) after_clip->num_frames_ * after_clip->channels_;
    ASSERT_EQ (total_after_frames, total_orig_frames);
    ASSERT_TRUE (audio_frames_equal (
      after_clip->frames_.getReadPointer (0), &orig_frames[0],
      total_orig_frames, 0.0001f));
  }

  UNDO_MANAGER->redo ();
}

// below pass

TEST_F (ZrythmFixture, MoveAudioRegionAndLowerBPM)
{
  const auto frames_per_tick_at_start = AUDIO_ENGINE->frames_per_tick_;

  auto check_actions = [&] (size_t num_actions_till_now) {
    ASSERT_SIZE_EQ (UNDO_MANAGER->undo_stack_->actions_, num_actions_till_now);
    const auto &undo_stack_actions = UNDO_MANAGER->undo_stack_->actions_;

    auto require_frames_per_tick_eq_to_start = [&] (size_t idx) {
      ASSERT_DOUBLE_EQ (
        undo_stack_actions[idx]->frames_per_tick_, frames_per_tick_at_start);
    };

    if (num_actions_till_now > 0)
      {
        require_frames_per_tick_eq_to_start (0);
        require_frames_per_tick_eq_to_start (1);
        ASSERT_EQ (undo_stack_actions[1]->num_actions_, 2);
      }
    if (num_actions_till_now > 2)
      {
        require_frames_per_tick_eq_to_start (2);
        ASSERT_EQ (undo_stack_actions[2]->num_actions_, 1);
      }
    if (num_actions_till_now > 3)
      {
        require_frames_per_tick_eq_to_start (3);
        ASSERT_EQ (undo_stack_actions[3]->num_actions_, 1);
      }
  };

  /* create audio track with region */
  Position       pos;
  int            track_pos = TRACKLIST->tracks_.size ();
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test.wav");
  ASSERT_NO_THROW (Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, track_pos, 1, -1, nullptr));
  auto track = TRACKLIST->get_track<AudioTrack> (track_pos);
  ASSERT_NONNULL (track);
  check_actions (2);

  /* move the region */
  track->lanes_[0]->regions_[0]->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, true, MOVE_TICKS, 0, 0, nullptr, false));
  check_actions (3);

  for (int i = 0; i < 2; ++i)
    {
      check_actions (3);
      float bpm_diff = (i == 1) ? 20.f : 40.f;

      /* lower BPM and attempt to save */
      bpm_t bpm_before = P_TEMPO_TRACK->get_current_bpm ();
      auto  audio_track = TRACKLIST->get_track<AudioTrack> (5);
      auto &r = audio_track->lanes_[0]->regions_[0];
      auto &clip = AUDIO_ENGINE->pool_->clips_[r->pool_id_];
      z_debug (
        "before | r size: {} (ticks {:f}), clip size {}",
        r->get_length_in_frames (), r->get_length_in_ticks (),
        clip->num_frames_);
      r->validate (true, 0);
      P_TEMPO_TRACK->set_bpm (
        bpm_before - bpm_diff, bpm_before, Z_F_NOT_TEMPORARY, false);
      check_actions (4);
      z_debug (
        "after | r size: {} (ticks {:f}), clip size {}",
        r->get_length_in_frames (), r->get_length_in_ticks (),
        clip->num_frames_);
      r->validate (true, 0);
      test_project_save_and_reload ();
      check_actions (4);

      /* undo lowering BPM */
      UNDO_MANAGER->undo ();
      check_actions (3);
    }
}

TEST_F (ArrangerSelectionsFixture, DeleteChords)
{
  ASSERT_TRUE (P_CHORD_TRACK->validate ());
  auto &r = P_CHORD_TRACK->regions_[0];
  ASSERT_TRUE (r->validate (true, 0));

  /* add another chord */
  auto c = std::make_shared<ChordObject> (r->id_, 2, 2);
  r->append_object (c);
  CHORD_SELECTIONS->add_object_ref (c);
  perform_create (CHORD_SELECTIONS);

  /* delete the first chord */
  CHORD_SELECTIONS->clear ();
  CHORD_SELECTIONS->add_object_ref (r->chord_objects_[0]);
  perform_delete (CHORD_SELECTIONS);
  ASSERT_TRUE (r->validate (true, 0));

  ASSERT_NO_THROW (
    UNDO_MANAGER->undo (); UNDO_MANAGER->redo (); UNDO_MANAGER->undo ();
    UNDO_MANAGER->undo (); UNDO_MANAGER->redo (); UNDO_MANAGER->undo (););
}

TEST_F (ArrangerSelectionsFixture, CreateTimeline)
{
  /* do create */
  perform_create (TL_SELECTIONS);

  /* check */
  ASSERT_EQ (TL_SELECTIONS->get_num_objects (), TOTAL_TL_SELECTIONS);
  ASSERT_EQ (MIDI_SELECTIONS->get_num_objects (), 1);
  check_vs_original_state (true);
  check_has_single_undo ();

  /* undo and check that the objects are deleted */
  UNDO_MANAGER->undo ();
  ASSERT_EQ (MIDI_SELECTIONS->get_num_objects (), 0);
  check_timeline_objects_deleted (true);

  /* redo and check that the objects are there */
  UNDO_MANAGER->redo ();
  ASSERT_EQ (TL_SELECTIONS->get_num_objects (), TOTAL_TL_SELECTIONS);
  check_vs_original_state (true);
  check_has_single_undo ();
}

TEST_F (ArrangerSelectionsFixture, DeleteTimelineSelections)
{
  /* do delete */
  ASSERT_NO_THROW (perform_delete (TL_SELECTIONS));

  ASSERT_NULL (CLIP_EDITOR->get_region ());

  /* check */
  check_timeline_objects_deleted (false);
  ASSERT_EQ (MIDI_SELECTIONS->get_num_objects (), 0);
  ASSERT_EQ (CHORD_SELECTIONS->get_num_objects (), 0);
  ASSERT_EQ (AUTOMATION_SELECTIONS->get_num_objects (), 0);

  /* undo and check that the objects are created */
  ASSERT_NO_THROW (UNDO_MANAGER->undo ());
  ASSERT_EQ (TL_SELECTIONS->get_num_objects (), TOTAL_TL_SELECTIONS);
  check_vs_original_state (true);
  check_has_single_redo ();

  /* redo and check that the objects are gone */
  UNDO_MANAGER->redo ();
  ASSERT_EQ (TL_SELECTIONS->get_num_objects (), 0);
  ASSERT_EQ (MIDI_SELECTIONS->get_num_objects (), 0);
  ASSERT_EQ (CHORD_SELECTIONS->get_num_objects (), 0);
  ASSERT_EQ (AUTOMATION_SELECTIONS->get_num_objects (), 0);
  check_timeline_objects_deleted (false);

  ASSERT_NULL (CLIP_EDITOR->get_region ());

  /* undo again to prepare for next test */
  UNDO_MANAGER->undo ();
  ASSERT_EQ (TL_SELECTIONS->get_num_objects (), TOTAL_TL_SELECTIONS);
  check_vs_original_state (true);
  check_has_single_redo ();
}

TEST_F (ZrythmFixture, MoveAudioRegionAndLowerSampleRate)
{
  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR, G_DIR_SEPARATOR_S, "test.wav");

  /* create audio track with region */
  Position       pos;
  int            track_pos = TRACKLIST->tracks_.size ();
  FileDescriptor file (audio_file_path);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, track_pos, 1, -1, nullptr);
  auto track = TRACKLIST->get_track<AudioTrack> (track_pos);
  ASSERT_NONNULL (track);

  /* move the region */
  track->lanes_[0]->regions_[0]->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
      *TL_SELECTIONS, true, MOVE_TICKS, 0, 0, nullptr, false));

  for (int i = 0; i < 4; i++)
    {
      /* save the project */
      ASSERT_NO_THROW (PROJECT->save (PROJECT->dir_, 0, 0, false));
      auto prj_file = fs::path (PROJECT->dir_) / PROJECT_FILE;

      /* adjust the samplerate to be given at startup */
      zrythm_app->samplerate_ = (int) AUDIO_ENGINE->sample_rate_ / 2;

      AUDIO_ENGINE->activate (false);
      PROJECT.reset ();

      /* reload */
      test_project_reload (prj_file);
    }
}

TEST_F (ArrangerSelectionsFixture, MoveTimlineSelections)
{
  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      int track_diff = i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
        }

      /* check undo/redo stacks */
      ASSERT_EMPTY (*UNDO_MANAGER->undo_stack_);
      ASSERT_SIZE_EQ (*UNDO_MANAGER->redo_stack_, i ? 1 : 0);

      /* do move ticks */
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
          *TL_SELECTIONS, true, MOVE_TICKS, track_diff, 0, nullptr, false));

      /* check */
      check_after_move_timeline (i);

      /* undo and check that the objects are at their original state*/
      UNDO_MANAGER->undo ();
      ASSERT_EQ (TL_SELECTIONS->get_num_objects (), i ? 2 : TOTAL_TL_SELECTIONS);

      check_vs_original_state (i ? false : true);
      check_has_single_redo ();

      /* redo and check that the objects are moved
       * again */
      UNDO_MANAGER->redo ();
      check_after_move_timeline (i);

      /* undo again to prepare for next test */
      UNDO_MANAGER->undo ();
      if (track_diff)
        {
          ASSERT_EQ (TL_SELECTIONS->get_num_objects (), 2);
        }
    }
}