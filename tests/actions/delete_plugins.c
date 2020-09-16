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

#include "actions/delete_plugins_action.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/control_port.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#include <glib.h>

static void
test_midi_fx_slot_deletion (void)
{
  test_helper_zrythm_init ();

  /* create MIDI track */
  UndoableAction * ua =
    create_tracks_action_new_midi (
      TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

#ifdef HAVE_MIDI_CC_MAP
  /* add plugin to slot */
  int slot = 0;
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      MIDI_CC_MAP_BUNDLE, MIDI_CC_MAP_URI, false);
  ua =
    create_plugins_action_new (
      descr, PLUGIN_SLOT_MIDI_FX,
      TRACKLIST->num_tracks - 1, slot, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* delete slot and undo/redo */
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->midi_fx[slot];
  plugin_select (pl, F_SELECT, F_EXCLUSIVE);
  ua = delete_plugins_action_new (MIXER_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  undo_manager_undo (UNDO_MANAGER);

  undo_manager_redo (UNDO_MANAGER);
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/delete_plugins/"

  g_test_add_func (
    TEST_PREFIX "test MIDI fx slot deletion",
    (GTestFunc) test_midi_fx_slot_deletion);

  return g_test_run ();
}
