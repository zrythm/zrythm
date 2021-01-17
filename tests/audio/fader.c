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

#include "actions/tracklist_selections.h"
#include "audio/fader.h"
#include "audio/midi_event.h"
#include "audio/router.h"
#include "utils/math.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

static void
test_fader_process_with_instrument (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      pl_bundle, pl_uri, with_carla);

  /* create instrument track */
  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_INSTRUMENT,
      descr, NULL, TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* send a note then wait for playback */
  midi_events_add_note_on (
    track->processor->midi_in->midi_events,
    1, 82, 74, 2, true);

  /* run engine twice (running one is not enough to
   * make the note make sound) */
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  /* test fader */
  zix_sem_wait (&ROUTER->graph_access);
  bool has_signal = false;
  Port * l = track->channel->fader->stereo_out->l;
  /*g_warn_if_reached ();*/
  for (nframes_t i = 0;
       i < AUDIO_ENGINE->block_length; i++)
    {
      if (l->buf[i] > 0.0001f)
        {
          has_signal = true;
          break;
        }
    }
  g_assert_true (has_signal);
  zix_sem_post (&ROUTER->graph_access);
}

static void
test_fader_process ()
{
  test_helper_zrythm_init ();

#ifdef HAVE_HELM
  test_fader_process_with_instrument (
    HELM_BUNDLE, HELM_URI, false);
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/fader/"

  g_test_add_func (
    TEST_PREFIX "test fader process",
    (GTestFunc) test_fader_process);

  return g_test_run ();
}
