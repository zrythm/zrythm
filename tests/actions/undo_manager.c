// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <math.h>

#include "actions/undo_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <locale.h>

static void
perform_create_region_action (void)
{
  Position p1, p2;
  position_set_to_bar (&p1, 1);
  position_set_to_bar (&p2, 2);
  int       track_pos = TRACKLIST->num_tracks - 1;
  Track *   track = TRACKLIST->tracks[track_pos];
  ZRegion * r = automation_region_new (
    &p1, &p2, track_get_name_hash (track), 0, 0);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  track = TRACKLIST->tracks[track_pos];
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at = atl->ats[0];
  track_add_region (
    track, r, at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    (ArrangerSelections *) TL_SELECTIONS, NULL);
}

static void
test_perform_many_actions (void)
{
  test_helper_zrythm_init ();

  for (int i = 0; !undo_stack_is_full (
         UNDO_MANAGER->undo_stack);
       i++)
    {
      if (i % 2 == 0)
        {
          track_create_empty_with_action (
            TRACK_TYPE_AUDIO_BUS, NULL);
        }
      else if (i % 13 == 0)
        {
          undo_manager_undo (UNDO_MANAGER, NULL);
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
  track_create_empty_with_action (
    TRACK_TYPE_MIDI, NULL);
  perform_create_region_action ();
  perform_create_region_action ();

  for (
    int i = 0;
    !undo_stack_is_full (UNDO_MANAGER->redo_stack)
    || undo_stack_is_empty (
      UNDO_MANAGER->redo_stack);
    i++)
    {
      if (undo_stack_is_empty (
            UNDO_MANAGER->redo_stack))
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
          undo_manager_redo (UNDO_MANAGER, NULL);
        }
    }

  test_helper_zrythm_cleanup ();
}

static void
test_multi_actions (void)
{
  test_helper_zrythm_init ();

  int       max_actions = 3;
  const int num_tracks_at_start =
    TRACKLIST->num_tracks;
  for (int i = 0; i < max_actions; i++)
    {
      track_create_empty_with_action (
        TRACK_TYPE_AUDIO_BUS, NULL);
      if (i == 2)
        {
          UndoableAction * ua =
            undo_manager_get_last_action (
              UNDO_MANAGER);
          ua->num_actions = max_actions;
        }
    }

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==,
    num_tracks_at_start + max_actions);
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==,
    max_actions - 1);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==, -1);

  undo_manager_undo (UNDO_MANAGER, NULL);

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==, num_tracks_at_start);
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==, -1);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==,
    max_actions - 1);

  undo_manager_redo (UNDO_MANAGER, NULL);

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==,
    num_tracks_at_start + max_actions);
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==,
    max_actions - 1);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==, -1);

  undo_manager_undo (UNDO_MANAGER, NULL);

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==, num_tracks_at_start);
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==, -1);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==,
    max_actions - 1);

  test_helper_zrythm_cleanup ();
}

static void
test_fill_stack (void)
{
  test_helper_zrythm_init ();

  int max_len =
    UNDO_MANAGER->undo_stack->stack->max_length;
  for (int i = 0; i < max_len + 8; i++)
    {
      track_create_empty_with_action (
        TRACK_TYPE_AUDIO_BUS, NULL);
    }

  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/undo manager/"

  g_test_add_func (
    TEST_PREFIX "test multi actions",
    (GTestFunc) test_multi_actions);
  g_test_add_func (
    TEST_PREFIX "test fill stack",
    (GTestFunc) test_fill_stack);
  g_test_add_func (
    TEST_PREFIX "test perform many actions",
    (GTestFunc) test_perform_many_actions);

  return g_test_run ();
}
