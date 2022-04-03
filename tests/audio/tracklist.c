/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <glib.h>

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <locale.h>

static void
create_automation_region (int track_pos)
{
  Track * track = TRACKLIST->tracks[track_pos];

  Position start, end;
  position_set_to_bar (&start, 1);
  position_set_to_bar (&end, 3);
  ZRegion * region = automation_region_new (
    &start, &end, track_get_name_hash (track), 0, 0);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  track_add_region (
    track, region, atl->ats[0], 0, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) region, F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    (ArrangerSelections *) TL_SELECTIONS, NULL);

  AutomationPoint * ap =
    automation_point_new_float (0.1f, 0.1f, &start);
  automation_region_add_ap (
    region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    (ArrangerSelections *) AUTOMATION_SELECTIONS,
    NULL);
}

static void
test_swap_with_automation_regions (void)
{
  test_helper_zrythm_init ();

  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, NULL, PLAYHEAD,
    TRACKLIST->num_tracks, 1, NULL);

  create_automation_region (
    TRACKLIST->num_tracks - 1);

  track_create_empty_with_action (
    TRACK_TYPE_MIDI, NULL);

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
  tracklist_selections_action_perform_move (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    track1->pos, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  test_project_save_and_reload ();

  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_handle_drop_empty_midi_file (void)
{
  test_helper_zrythm_init ();

  char * path = g_build_filename (
    TESTS_SRCDIR, "empty_midi_file_type1.mid", NULL);
  SupportedFile * file =
    supported_file_new_from_path (path);
  g_free (path);

  tracklist_handle_file_drop (
    TRACKLIST, NULL, file, NULL, NULL, PLAYHEAD,
    true);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  yaml_set_log_level (CYAML_LOG_INFO);

#define TEST_PREFIX "/audio/tracklist/"

  g_test_add_func (
    TEST_PREFIX "test handle drop empty midi file",
    (GTestFunc) test_handle_drop_empty_midi_file);
  g_test_add_func (
    TEST_PREFIX "test swap with automation regions",
    (GTestFunc) test_swap_with_automation_regions);

  return g_test_run ();
}
