// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/arranger_selections.h"
#include "actions/undo_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("actions/undo manager");

static void
perform_create_region_action ()
{
  Position p1, p2;
  p1.set_to_bar (1);
  p2.set_to_bar (2);
  int  track_pos = TRACKLIST->get_num_tracks () - 1;
  auto track = TRACKLIST->get_track<AutomatableTrack> (track_pos);
  REQUIRE_NONNULL (track);
  auto r =
    std::make_shared<AutomationRegion> (p1, p2, track->get_name_hash (), 0, 0);
  const auto &atl = track->get_automation_tracklist ();
  const auto &at = atl.ats_.front ();
  track->add_region (r, at.get (), -1, F_GEN_NAME, F_NO_PUBLISH_EVENTS);
  r->select (F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::CreateAction> (*TL_SELECTIONS));
}

TEST_CASE ("perform many actions")
{
  test_helper_zrythm_init ();

  for (int i = 0; !UNDO_MANAGER->undo_stack_->is_full (); ++i)
    {
      if (i % 2 == 0)
        {
          Track::create_empty_with_action<AudioBusTrack> ();
        }
      else if (i % 13 == 0)
        {
          UNDO_MANAGER->undo ();
        }
      else if (i % 17 == 0)
        {
          test_project_save_and_reload ();
        }
      else
        {
          perform_create_region_action ();
        }
    }
  Track::create_empty_with_action<MidiTrack> ();
  perform_create_region_action ();
  perform_create_region_action ();

  for (
    int i = 0;
    !UNDO_MANAGER->redo_stack_->is_full ()
    || UNDO_MANAGER->redo_stack_->empty ();
    i++)
    {
      if (UNDO_MANAGER->redo_stack_->empty ())
        {
          break;
        }

      if (i % 17 == 0)
        {
          test_project_save_and_reload ();
        }
      else if (i % 47 == 0)
        {
          perform_create_region_action ();
        }
      else
        {
          UNDO_MANAGER->redo ();
        }
    }

  test_helper_zrythm_cleanup ();
}

TEST_CASE ("multi actions")
{
  test_helper_zrythm_init ();

  int       max_actions = 3;
  const int num_tracks_at_start = TRACKLIST->get_num_tracks ();
  for (int i = 0; i < max_actions; i++)
    {
      Track::create_empty_with_action<AudioBusTrack> ();
      if (i == 2)
        {
          auto ua = UNDO_MANAGER->get_last_action ();
          ua->num_actions_ = max_actions;
        }
    }

  REQUIRE_SIZE_EQ (TRACKLIST->tracks_, num_tracks_at_start + max_actions);
  REQUIRE_SIZE_EQ (*UNDO_MANAGER->undo_stack_, max_actions);
  REQUIRE_EMPTY (*UNDO_MANAGER->redo_stack_);

  UNDO_MANAGER->undo ();

  REQUIRE_SIZE_EQ (TRACKLIST->tracks_, num_tracks_at_start);
  REQUIRE_EMPTY (*UNDO_MANAGER->undo_stack_);
  REQUIRE_SIZE_EQ (*UNDO_MANAGER->redo_stack_, max_actions);

  UNDO_MANAGER->redo ();

  REQUIRE_SIZE_EQ (TRACKLIST->tracks_, num_tracks_at_start + max_actions);
  REQUIRE_SIZE_EQ (*UNDO_MANAGER->undo_stack_, max_actions);
  REQUIRE_EMPTY (*UNDO_MANAGER->redo_stack_);

  UNDO_MANAGER->undo ();

  REQUIRE_SIZE_EQ (TRACKLIST->tracks_, num_tracks_at_start);
  REQUIRE_EMPTY (*UNDO_MANAGER->undo_stack_);
  REQUIRE_SIZE_EQ (*UNDO_MANAGER->redo_stack_, max_actions);

  test_helper_zrythm_cleanup ();
}

TEST_CASE ("fill stack")
{
  test_helper_zrythm_init ();

  const auto max_len = UNDO_MANAGER->undo_stack_->max_size_;
  for (size_t i = 0; i < max_len + 8; ++i)
    {
      Track::create_empty_with_action<AudioBusTrack> ();
    }

  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}

TEST_SUITE_END;