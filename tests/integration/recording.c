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

#include "audio/engine_dummy.h"
#include "audio/master_track.h"
#include "audio/midi.h"
#include "audio/midi_track.h"
#include "audio/recording_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>

#define CYCLE_SIZE 100

static void
test_recording_takes (
  Track * ins_track,
  Track * audio_track,
  Track * master_track)
{
  /* ---- no loop no punch ---- */

  TRANSPORT->recording = true;
  transport_request_roll (TRANSPORT);

  /* disable loop & punch */
  transport_set_loop (TRANSPORT, false);
  transport_set_punch_mode_enabled (TRANSPORT, false);

  /* move playhead to 2.1.1.0 */
  Position pos;
  position_set_to_bar (&pos, 2);
  transport_set_playhead_pos (TRANSPORT, &pos);

  /* enable recording for audio & ins track */
  track_set_recording (ins_track, true, false);
  track_set_recording (audio_track, true, false);

  /* enable recording for master track automation */
  AutomationTrack * at =
    master_track->automation_tracklist.ats[0];
  at->automation_mode = AUTOMATION_MODE_RECORD;
  at->record_mode = AUTOMATION_RECORD_MODE_LATCH;

  /* run the engine for 1 cycle */
  engine_process (AUDIO_ENGINE, CYCLE_SIZE);
  recording_manager_process_events (
    RECORDING_MANAGER);

  ZRegion * region;
  ArrangerObject * r_obj;

  /* assert that MIDI events are created */
  g_assert_cmpint (
    ins_track->lanes[0]->num_regions, ==, 1);
  region = ins_track->lanes[0]->regions[0];
  r_obj = (ArrangerObject *) region;
  position_set_to_pos (
    &pos, &TRANSPORT->playhead_pos);
  g_assert_cmppos (&pos, &r_obj->end_pos);
  position_add_frames (&pos, - CYCLE_SIZE);
  g_assert_cmppos (&pos, &r_obj->pos);
  position_from_frames (&pos, CYCLE_SIZE);
  g_assert_cmppos (&pos, &r_obj->loop_end_pos);
}

#ifdef HAVE_HELM
static void
test_recording ()
{
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* create an instrument track for testing */
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      HELM_BUNDLE, HELM_URI, false);

  /* fix the descriptor (for some reason lilv
   * reports it as Plugin instead of Instrument if
   * you don't do lilv_world_load_all) */
  descr->category = PC_INSTRUMENT;
  g_free (descr->category_str);
  descr->category_str =
    plugin_descriptor_category_to_string (
      descr->category);

  /* create an instrument track from helm */
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_INSTRUMENT, descr, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * ins_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* create an audio track */
  ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO, NULL, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* get master track */
  Track * master_track = P_MASTER_TRACK;

  test_recording_takes (
    ins_track, audio_track, master_track);

  test_helper_zrythm_cleanup ();
}
#endif

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/integration/recording/"

#ifdef HAVE_HELM
  g_test_add_func (
    TEST_PREFIX "test_recording",
    (GTestFunc) test_recording);
#endif

  return g_test_run ();
}
