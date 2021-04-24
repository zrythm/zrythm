/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, true, with_carla, 1);

  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* send a note then wait for playback */
  midi_events_add_note_on (
    track->processor->midi_in->midi_events,
    1, 82, 74, 2, true);

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

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

static bool
track_has_sound (
  Track * track)
{
  for (nframes_t i = 0;
       i < AUDIO_ENGINE->block_length; i++)
    {
      if (fabsf (
            track->channel->fader->
              stereo_out->l->buf[i]) > 0.0001f)
        {
          return true;
        }
    }
  return false;
}

static void
test_track_has_sound (
  Track * track,
  bool    expect_sound)
{
  Position pos;
  position_set_to_bar (&pos, 1);
  transport_set_playhead_pos (TRANSPORT, &pos);
  transport_request_roll (TRANSPORT);

  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  g_assert_true (
    track_has_sound (track) == expect_sound);

  transport_request_pause (TRANSPORT);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
}

static void
test_solo ()
{
  test_helper_zrythm_init ();

  /* create audio track */
  char * filepath =
    g_build_filename (
      TESTS_SRCDIR, "test.wav", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO, NULL, file,
      TRACKLIST->num_tracks, PLAYHEAD, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* create audio track 2 */
  ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO, NULL, file,
      TRACKLIST->num_tracks, PLAYHEAD, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_track2 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* create group track */
  ua =
    tracklist_selections_action_new_create_audio_group (
      TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * group_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* route audio tracks to group track */
  track_select (
    audio_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    tracklist_selections_action_new_edit_direct_out(
      TRACKLIST_SELECTIONS, group_track);
  undo_manager_perform (UNDO_MANAGER, ua);
  track_select (
    audio_track2, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    tracklist_selections_action_new_edit_direct_out(
      TRACKLIST_SELECTIONS, group_track);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* test solo group makes sound */
  track_set_soloed (
    group_track, F_SOLO, F_TRIGGER_UNDO,
    F_NO_PUBLISH_EVENTS);
  test_track_has_sound (P_MASTER_TRACK, true);
  test_track_has_sound (group_track, true);
  test_track_has_sound (audio_track, true);
  test_track_has_sound (audio_track2, true);
  undo_manager_undo (UNDO_MANAGER);

  /* test solo audio track makes sound */
  track_set_soloed (
    audio_track, F_SOLO, F_TRIGGER_UNDO,
    F_NO_PUBLISH_EVENTS);
  test_track_has_sound (P_MASTER_TRACK, true);
  test_track_has_sound (group_track, true);
  test_track_has_sound (audio_track, true);
  test_track_has_sound (audio_track2, false);
  undo_manager_undo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/fader/"

  g_test_add_func (
    TEST_PREFIX "test solo",
    (GTestFunc) test_solo);
  g_test_add_func (
    TEST_PREFIX "test fader process",
    (GTestFunc) test_fader_process);

  return g_test_run ();
}
