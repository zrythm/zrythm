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

#include <math.h>

#include "audio/automation_region.h"
#include "audio/tracklist.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

static void
create_automation_region (
  int track_pos)
{
  Track * track = TRACKLIST->tracks[track_pos];

  Position start, end;
  position_set_to_bar (&start, 1);
  position_set_to_bar (&end, 3);
  ZRegion * region =
    automation_region_new (
      &start, &end, track->pos, 0, 0);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  track_add_region (
    track, region, atl->ats[0], 0, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) region, F_SELECT, F_NO_APPEND);
  UndoableAction * ua =
    arranger_selections_action_new_create (
      (ArrangerSelections *) TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  AutomationPoint * ap =
    automation_point_new_float (
      0.1f, 0.1f, &start);
  automation_region_add_ap (
    region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_NO_APPEND);
  ua =
    arranger_selections_action_new_create (
      (ArrangerSelections *) AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
}

static void
test_swap_with_automation_regions ()
{
  test_helper_zrythm_init ();

  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  create_automation_region (
    TRACKLIST->num_tracks - 1);

  ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_MIDI, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  create_automation_region (
    TRACKLIST->num_tracks - 1);

  /* swap tracks */
  Track * track1 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2];
  Track * track2 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track2, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    tracklist_selections_action_new_move (
      TRACKLIST_SELECTIONS, track1->pos);
  undo_manager_perform (UNDO_MANAGER, ua);

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);

  test_project_save_and_reload ();

  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/tracklist/"

  g_test_add_func (
    TEST_PREFIX "test swap with automation regions",
    (GTestFunc) test_swap_with_automation_regions);

  return g_test_run ();
}
