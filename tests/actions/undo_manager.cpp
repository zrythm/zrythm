// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/project/project.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

static void
perform_create_region_action ()
{
  Position p1, p2;
  p1.set_to_bar (1);
  p2.set_to_bar (2);
  int  track_pos = TRACKLIST->get_num_tracks () - 1;
  auto track = TRACKLIST->get_track<AutomatableTrack> (track_pos);
  ASSERT_NONNULL (track);
  auto r =
    std::make_shared<AutomationRegion> (p1, p2, track->get_name_hash (), 0, 0);
  const auto &atl = track->get_automation_tracklist ();
  const auto &at = atl.ats_.front ();
  track->add_region (r, at.get (), -1, true, false);
  r->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));
}

TEST_F (ZrythmFixture, PerformManyActions)
{
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
}

TEST_F (ZrythmFixture, MultiActions)
{
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

  ASSERT_SIZE_EQ (TRACKLIST->tracks_, num_tracks_at_start + max_actions);
  ASSERT_SIZE_EQ (*UNDO_MANAGER->undo_stack_, max_actions);
  ASSERT_EMPTY (*UNDO_MANAGER->redo_stack_);

  UNDO_MANAGER->undo ();

  ASSERT_SIZE_EQ (TRACKLIST->tracks_, num_tracks_at_start);
  ASSERT_EMPTY (*UNDO_MANAGER->undo_stack_);
  ASSERT_SIZE_EQ (*UNDO_MANAGER->redo_stack_, max_actions);

  UNDO_MANAGER->redo ();

  ASSERT_SIZE_EQ (TRACKLIST->tracks_, num_tracks_at_start + max_actions);
  ASSERT_SIZE_EQ (*UNDO_MANAGER->undo_stack_, max_actions);
  ASSERT_EMPTY (*UNDO_MANAGER->redo_stack_);

  UNDO_MANAGER->undo ();

  ASSERT_SIZE_EQ (TRACKLIST->tracks_, num_tracks_at_start);
  ASSERT_EMPTY (*UNDO_MANAGER->undo_stack_);
  ASSERT_SIZE_EQ (*UNDO_MANAGER->redo_stack_, max_actions);
}

TEST_F (ZrythmFixture, FillStack)
{
  const auto max_len = UNDO_MANAGER->undo_stack_->max_size_;
  for (size_t i = 0; i < max_len + 8; ++i)
    {
      Track::create_empty_with_action<AudioBusTrack> ();
    }

  test_project_save_and_reload ();
}
