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

#include "zrythm-config.h"
#include "zrythm-test-config.h"

#include "actions/edit_tracks_action.h"
#include "actions/undoable_action.h"
#include "audio/master_track.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#include <glib.h>

static void
_test_edit_tracks (
  EditTracksActionType type,
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  UndoableAction * action;

  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      pl_bundle, pl_uri, with_carla);

  /* create a track with an instrument */
  action =
    create_tracks_action_new (
      is_instrument ?
        TRACK_TYPE_INSTRUMENT :
        TRACK_TYPE_AUDIO_BUS,
      descr, NULL, 3, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, action);
  Track * ins_track = TRACKLIST->tracks[3];
  if (is_instrument)
    {
      g_assert_true (
        ins_track->type == TRACK_TYPE_INSTRUMENT);

      g_assert_true (
        ins_track->channel->instrument->instantiated);
      g_assert_true (
        ins_track->channel->instrument->activated);
    }
  else
    {
      g_assert_true (
        ins_track->channel->inserts[0]->instantiated);
      g_assert_true (
        ins_track->channel->inserts[0]->activated);
    }

  track_select (
    ins_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  switch (type)
    {
    case EDIT_TRACK_ACTION_TYPE_MUTE:
      {
        UndoableAction * ua =
          edit_tracks_action_new_mute (
            TRACKLIST_SELECTIONS, true);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        if (is_instrument)
          {
            g_assert_true (
              ins_track->channel->instrument->
                instantiated);
            g_assert_true (
              ins_track->channel->instrument->
                activated);
          }
        else
          {
            g_assert_true (
              ins_track->channel->inserts[0]->
                instantiated);
            g_assert_true (
              ins_track->channel->inserts[0]->
                activated);
          }
      }
      break;
    case EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
      {
        if (!is_instrument)
          break;

        /* create a MIDI track */
        action =
          create_tracks_action_new (
            TRACK_TYPE_MIDI,
            descr, NULL, 2, NULL, 1);
        undo_manager_perform (UNDO_MANAGER, action);
        Track * midi_track = TRACKLIST->tracks[2];
        track_select (
          midi_track, F_SELECT, F_EXCLUSIVE,
          F_NO_PUBLISH_EVENTS);

        g_assert_true (
          !midi_track->channel->has_output);

        /* change the direct out to the
         * instrument */
        UndoableAction * ua =
          edit_tracks_action_new_direct_out (
            TRACKLIST_SELECTIONS, ins_track);
        undo_manager_perform (UNDO_MANAGER, ua);

        /* verify direct out established */
        g_assert_true (
          midi_track->channel->has_output);
        g_assert_cmpint (
          midi_track->channel->output_pos, ==,
          ins_track->pos);

        /* undo and re-verify */
        undo_manager_undo (UNDO_MANAGER);

        g_assert_true (
          !midi_track->channel->has_output);
      }
    default:
      break;
    }

  /* let the engine run */
  g_usleep (1000000);
}

static void __test_edit_tracks (bool with_carla)
{
  test_helper_zrythm_init ();

  for (EditTracksActionType i =
         EDIT_TRACK_ACTION_TYPE_SOLO;
       i <= EDIT_TRACK_ACTION_TYPE_DIRECT_OUT; i++)
    {
      /* only check mute, solo and direct out for
       * now */
      if (i != EDIT_TRACK_ACTION_TYPE_SOLO &&
          i != EDIT_TRACK_ACTION_TYPE_MUTE &&
          i != EDIT_TRACK_ACTION_TYPE_DIRECT_OUT)
        {
          continue;
        }

#ifdef HAVE_HELM
      _test_edit_tracks (
        i, HELM_BUNDLE, HELM_URI, true, with_carla);
#endif
#ifdef HAVE_LSP_COMPRESSOR
      _test_edit_tracks (
        i, LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
        false, with_carla);
#endif
    }

  test_helper_zrythm_cleanup ();
}

static void test_edit_tracks ()
{
  __test_edit_tracks (false);
#ifdef HAVE_CARLA
  __test_edit_tracks (true);
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/edit_tracks/"

  g_test_add_func (
    TEST_PREFIX "test_edit_tracks",
    (GTestFunc) test_edit_tracks);

  return g_test_run ();
}
