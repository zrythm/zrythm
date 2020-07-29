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

#include "actions/delete_tracks_action.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/control_port.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#include <glib.h>

static int num_master_children = 0;

static void
_test_copy_plugins (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      pl_bundle, pl_uri, with_carla);

  if (is_instrument)
    {
      /* fix the descriptor (for some reason lilv
       * reports it as Plugin instead of Instrument if
       * you don't do lilv_world_load_all) */
      descr->category = PC_INSTRUMENT;
      g_free (descr->category_str);
      descr->category_str =
        plugin_descriptor_category_to_string (
          descr->category);
    }

  /* create the plugin track */
  UndoableAction * ua =
    create_tracks_action_new (
      is_instrument ?
        TRACK_TYPE_INSTRUMENT :
        TRACK_TYPE_AUDIO_BUS,
      descr, NULL, TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  num_master_children++;
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==,
    num_master_children);
  g_assert_cmpint (
    P_MASTER_TRACK->children[
      num_master_children - 1], ==,
    TRACKLIST->num_tracks - 1);
  g_assert_cmpint (
    P_MASTER_TRACK->children[0], ==, 4);

  /* save and reload the project */
  test_project_save_and_reload ();

  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==,
    num_master_children);
  g_assert_cmpint (
    P_MASTER_TRACK->children[
      num_master_children - 1], ==,
    TRACKLIST->num_tracks - 1);
  g_assert_cmpint (
    P_MASTER_TRACK->children[0], ==, 4);

  /* select track */
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  ua =
    copy_tracks_action_new (
      TRACKLIST_SELECTIONS, TRACKLIST->num_tracks);
  undo_manager_perform (UNDO_MANAGER, ua);
  num_master_children++;
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==,
    num_master_children);
  g_assert_cmpint (
    P_MASTER_TRACK->children[
      num_master_children - 1], ==,
    TRACKLIST->num_tracks - 1);
  g_assert_cmpint (
    P_MASTER_TRACK->children[0], ==, 4);
  g_assert_cmpint (
    P_MASTER_TRACK->children[1], ==, 5);
  Track * new_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* if instrument, copy tracks, otherwise copy
   * plugins */
  if (is_instrument)
    {
      if (!with_carla)
        {
          g_assert_true (
            new_track->channel->instrument->lv2->ui);
        }
    }
  else
    {
      mixer_selections_clear (
        MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
      mixer_selections_add_slot (
        MIXER_SELECTIONS, track->channel,
        PLUGIN_SLOT_INSERT, 0);
      ua =
        copy_plugins_action_new (
          MIXER_SELECTIONS, PLUGIN_SLOT_INSERT,
          new_track, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
    }
}

static void
test_copy_plugins (void)
{
#ifdef HAVE_HELM
  _test_copy_plugins (
    HELM_BUNDLE, HELM_URI, true, false);
#ifdef HAVE_CARLA
  _test_copy_plugins (
    HELM_BUNDLE, HELM_URI, true, true);
#endif /* HAVE_CARLA */
#endif /* HAVE_HELM */
#ifdef HAVE_NO_DELAY_LINE
  _test_copy_plugins (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI,
    false, false);
#ifdef HAVE_CARLA
  _test_copy_plugins (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI,
    false, true);
#endif /* HAVE_CARLA */
#endif /* HAVE_NO_DELAY_LINE */
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/delete_tracks/"

  g_test_add_func (
    TEST_PREFIX "test copy plugins",
    (GTestFunc) test_copy_plugins);

  return g_test_run ();
}
