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

#include "actions/create_plugins_action.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/control_port.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "plugins/carla/carla_discovery.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#include <glib.h>

static void
_test_create_plugins (
  PluginProtocol prot,
  const char *   pl_bundle,
  const char *   pl_uri,
  bool           is_instrument,
  bool           with_carla)
{
  PluginDescriptor * descr = NULL;

  switch (prot)
    {
    case PROT_LV2:
      descr =
        plugin_descriptor_clone (
          test_plugin_manager_get_plugin_descriptor (
            pl_bundle, pl_uri, with_carla));
      break;
    case PROT_VST:
      descr =
        z_carla_discovery_create_vst_descriptor (
          pl_bundle, ARCH_64, PROT_VST);
      break;
    default:
      break;
    }

  UndoableAction * ua = NULL;
  if (is_instrument)
    {
      /* create an instrument track from helm */
      ua =
        create_tracks_action_new (
          TRACK_TYPE_INSTRUMENT, descr, NULL,
          TRACKLIST->num_tracks, NULL, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
    }
  else
    {
      /* create an audio fx track and add the
       * plugin */
      ua =
        create_tracks_action_new (
          TRACK_TYPE_AUDIO_BUS, NULL, NULL,
          TRACKLIST->num_tracks, NULL, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
      ua =
        create_plugins_action_new (
          descr, PLUGIN_SLOT_INSERT,
          TRACKLIST->num_tracks - 1, 0, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
    }

  plugin_descriptor_free (descr);

  /* let the engine run */
  g_usleep (1000000);

  test_project_save_and_reload ();

  int src_track_pos = TRACKLIST->num_tracks - 1;
  Track * src_track =
    TRACKLIST->tracks[src_track_pos];

  if (is_instrument)
    {
      g_assert_true (src_track->channel->instrument);
    }
  else
    {
      g_assert_true (src_track->channel->inserts[0]);
    }

  /* duplicate the track */
  track_select (
    src_track, F_SELECT, true, F_NO_PUBLISH_EVENTS);
  ua =
    copy_tracks_action_new (
      TRACKLIST_SELECTIONS, TRACKLIST->num_tracks);
  g_assert_true (
    track_verify_identifiers (src_track));
  undo_manager_perform (UNDO_MANAGER, ua);

  int dest_track_pos = TRACKLIST->num_tracks - 1;
  Track * dest_track =
    TRACKLIST->tracks[dest_track_pos];

  g_assert_true (
    track_verify_identifiers (src_track));
  g_assert_true (
    track_verify_identifiers (dest_track));

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);

  g_message ("letting engine run...");

  /* let the engine run */
  g_usleep (1000000);

  test_project_save_and_reload ();

  g_message ("done");
}

static void
test_create_plugins (void)
{
  test_helper_zrythm_init ();

  for (int i = 0; i < 2; i++)
    {
      if (i == 1)
        {
#ifdef HAVE_CARLA
#ifdef HAVE_NOIZEMAKER
          _test_create_plugins (
            PROT_VST, NOIZEMAKER_PATH, NULL,
            true, i);
#endif
#else
          break;
#endif
        }

#ifdef HAVE_HELM
      _test_create_plugins (
        PROT_LV2, HELM_BUNDLE, HELM_URI, true, i);
#endif
#ifdef HAVE_LSP_COMPRESSOR
      _test_create_plugins (
        PROT_LV2, LSP_COMPRESSOR_BUNDLE,
        LSP_COMPRESSOR_URI, false, i);
#endif
#ifdef HAVE_SHERLOCK_ATOM_INSPECTOR
      _test_create_plugins (
        PROT_LV2, SHERLOCK_ATOM_INSPECTOR_BUNDLE,
        SHERLOCK_ATOM_INSPECTOR_URI,
        false, i);
#endif
#ifdef HAVE_CARLA_RACK
      _test_create_plugins (
        PROT_LV2, CARLA_RACK_BUNDLE, CARLA_RACK_URI,
        true, i);
#endif
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/create_plugins/"

  g_test_add_func (
    TEST_PREFIX
    "test_create_plugins",
    (GTestFunc) test_create_plugins);

  test_helper_zrythm_cleanup ();

  return g_test_run ();
}
