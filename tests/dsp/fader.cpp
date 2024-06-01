// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "dsp/engine.h"
#include "dsp/fader.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/router.h"
#include "dsp/tracklist.h"
#include "project.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm_helper.h"

static void
test_fader_process_with_instrument (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, true, with_carla, 1);

  Track * track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];

  /* send a note then wait for playback */
  midi_events_add_note_on (
    track->processor->midi_in->midi_events_, 1, 82, 74, 2, true);

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* run engine twice (running one is not enough to
   * make the note make sound) */
  for (int i = 0; i < 2; i++)
    {
      engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
    }

  /* test fader */
  zix_sem_wait (&ROUTER->graph_access);
  bool  has_signal = false;
  Port &l = track->channel->fader->stereo_out->get_l ();
  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length; i++)
    {
      if (l.buf_[i] > 0.0001f)
        {
          has_signal = true;
          break;
        }
    }
  g_assert_true (has_signal);
  zix_sem_post (&ROUTER->graph_access);
}

static void
test_fader_process (void)
{
  test_helper_zrythm_init ();

  (void) test_fader_process_with_instrument;
  test_fader_process_with_instrument (
    TEST_INSTRUMENT_BUNDLE_URI, TEST_INSTRUMENT_URI, true);

  test_helper_zrythm_cleanup ();
}

static bool
track_has_sound (Track * track)
{
  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length; i++)
    {
      if (fabsf (track->channel->fader->stereo_out->get_l ().buf_[i]) > 0.0001f)
        {
          return true;
        }
    }
  return false;
}

static void
test_track_has_sound (Track * track, bool expect_sound)
{
  Position pos;
  position_set_to_bar (&pos, 1);
  transport_set_playhead_pos (TRANSPORT, &pos);
  transport_request_roll (TRANSPORT, true);

  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  g_assert_true (track_has_sound (track) == expect_sound);

  transport_request_pause (TRANSPORT, true);
  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
}

static void
test_solo (void)
{
  test_helper_zrythm_init ();

  /* create audio track */
  char *          filepath = g_build_filename (TESTS_SRCDIR, "test.wav", NULL);
  SupportedFile * file = supported_file_new_from_path (filepath);
  track_create_with_action (
    TrackType::TRACK_TYPE_AUDIO, NULL, file, PLAYHEAD,
    TRACKLIST->tracks.size (), 1, -1, NULL, NULL);
  Track * audio_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);

  /* create audio track 2 */
  track_create_with_action (
    TrackType::TRACK_TYPE_AUDIO, NULL, file, PLAYHEAD,
    TRACKLIST->tracks.size (), 1, -1, NULL, NULL);
  Track * audio_track2 = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);

  /* create group track */
  Track * group_track =
    track_create_empty_with_action (TrackType::TRACK_TYPE_AUDIO_GROUP, NULL);

  /* route audio tracks to group track */
  track_select (audio_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group_track, NULL);
  track_select (audio_track2, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group_track, NULL);

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* test solo group makes sound */
  track_set_soloed (
    group_track, F_SOLO, F_TRIGGER_UNDO, F_AUTO_SELECT, F_NO_PUBLISH_EVENTS);
  test_track_has_sound (P_MASTER_TRACK, true);
  test_track_has_sound (group_track, true);
  test_track_has_sound (audio_track, true);
  test_track_has_sound (audio_track2, true);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* test solo audio track makes sound */
  track_set_soloed (
    audio_track, F_SOLO, F_TRIGGER_UNDO, F_AUTO_SELECT, F_NO_PUBLISH_EVENTS);
  test_track_has_sound (P_MASTER_TRACK, true);
  test_track_has_sound (group_track, true);
  test_track_has_sound (audio_track, true);
  test_track_has_sound (audio_track2, false);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* test solo both audio tracks */
  track_select (audio_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (audio_track2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_edit_solo (
    TRACKLIST_SELECTIONS, F_SOLO, NULL);
  test_track_has_sound (P_MASTER_TRACK, true);
  test_track_has_sound (group_track, true);
  test_track_has_sound (audio_track, true);
  test_track_has_sound (audio_track2, true);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* test undo/redo */
  track_select (audio_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_edit_solo (
    TRACKLIST_SELECTIONS, F_SOLO, NULL);
  g_assert_true (track_get_soloed (audio_track));
  g_assert_false (track_get_soloed (audio_track2));
  track_select (audio_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (audio_track2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_edit_solo (
    TRACKLIST_SELECTIONS, F_SOLO, NULL);
  g_assert_true (track_get_soloed (audio_track));
  g_assert_true (track_get_soloed (audio_track2));
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (track_get_soloed (audio_track));
  g_assert_false (track_get_soloed (audio_track2));
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_true (track_get_soloed (audio_track));
  g_assert_true (track_get_soloed (audio_track2));

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/fader/"

  g_test_add_func (
    TEST_PREFIX "test fader process", (GTestFunc) test_fader_process);
  g_test_add_func (TEST_PREFIX "test solo", (GTestFunc) test_solo);

  return g_test_run ();
}
