/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "actions/undo_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

static void
perform_create_region_action ()
{
  Position p1, p2;
  position_set_to_bar (&p1, 1);
  position_set_to_bar (&p2, 2);
  int track_pos = TRACKLIST->num_tracks - 1;
  ZRegion * r =
    midi_region_new (
      &p1, &p2, track_pos, 0, 0);
  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  track_add_region (
    TRACKLIST->tracks[track_pos],
    r, NULL, 0, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  UndoableAction * ua =
    arranger_selections_action_new_create (
      (ArrangerSelections *) TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
}

static void
test_perform_many_actions ()
{
  test_helper_zrythm_init ();

  for (int i = 0;
       !undo_stack_is_full (UNDO_MANAGER->undo_stack);
       i++)
    {
      UndoableAction * ua = NULL;
      if (i % 2 == 0)
        {
          ua =
            tracklist_selections_action_new_create_midi (
              TRACKLIST->num_tracks, 1);
          undo_manager_perform (UNDO_MANAGER, ua);
        }
      else if (i % 13 == 0)
        {
          undo_manager_undo (UNDO_MANAGER);
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
  UndoableAction * ua =
    tracklist_selections_action_new_create_midi (
      TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  perform_create_region_action ();
  perform_create_region_action ();

  for (int i = 0;
       !undo_stack_is_full (
         UNDO_MANAGER->redo_stack) ||
       undo_stack_is_empty (
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
          undo_manager_redo (UNDO_MANAGER);
        }
    }

  test_helper_zrythm_cleanup ();
}

static void
test_multi_actions ()
{
  test_helper_zrythm_init ();

  int max_actions = 3;
  const int num_tracks_at_start =
    TRACKLIST->num_tracks;
  for (int i = 0; i < max_actions; i++)
    {
      UndoableAction * ua =
        tracklist_selections_action_new_create_midi (
          TRACKLIST->num_tracks, 1);
      if (i == 2)
        ua->num_actions = max_actions;
      undo_manager_perform (UNDO_MANAGER, ua);
    }

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==,
    num_tracks_at_start + max_actions);
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==,
    max_actions - 1);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==, -1);

  undo_manager_undo (UNDO_MANAGER);

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==, num_tracks_at_start);
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==, -1);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==,
    max_actions - 1);

  undo_manager_redo (UNDO_MANAGER);

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==,
    num_tracks_at_start + max_actions);
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==,
    max_actions - 1);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==, -1);

  undo_manager_undo (UNDO_MANAGER);

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
test_fill_stack ()
{
  test_helper_zrythm_init ();

  int max_len =
    UNDO_MANAGER->undo_stack->stack->max_length;
  for (int i = 0; i < max_len + 8; i++)
    {
      UndoableAction * ua = NULL;
      ua =
        tracklist_selections_action_new_create_audio_fx (
          NULL, TRACKLIST->num_tracks, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
    }

  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
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
