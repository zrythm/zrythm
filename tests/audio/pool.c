/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/tempo_track.h"
#include "audio/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "helpers/plugin_manager.h"
#include "helpers/project.h"
#include "helpers/zrythm.h"

#include <locale.h>

static void
test_remove_unused (void)
{
  test_helper_zrythm_init ();

  /* create many audio tracks with region to push
   * the first few off the undo stack */
  char * filepath = g_build_filename (
    TESTS_SRCDIR, "test_start_with_signal.mp3",
    NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  int num_tracks_before = TRACKLIST->num_tracks;
  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, PLAYHEAD,
    num_tracks_before, 1, NULL);

  /* delete the first track so that the clip for it
   * will no longer exist (pushed off undo stack) */
  track_select (
    TRACKLIST->tracks[num_tracks_before], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    NULL);

  /* create more tracks */
  for (int i = 0; i < (ZRYTHM->undo_stack_len + 4);
       i++)
    {
      track_create_with_action (
        TRACK_TYPE_AUDIO, NULL, file, PLAYHEAD,
        TRACKLIST->num_tracks, 1, NULL);
    }

  /* assert all clips still exist */
  g_assert_cmpint (
    AUDIO_POOL->num_clips, ==,
    ZRYTHM->undo_stack_len + 5);
  AudioClip * clip = AUDIO_POOL->clips[0];
  char * clip_path = audio_clip_get_path_in_pool (
    clip, F_NOT_BACKUP);
  g_assert_true (
    g_file_test (clip_path, G_FILE_TEST_EXISTS));
  AudioClip * clip2 = AUDIO_POOL->clips[1];
  char * clip_path2 = audio_clip_get_path_in_pool (
    clip2, F_NOT_BACKUP);
  g_assert_true (
    g_file_test (clip_path2, G_FILE_TEST_EXISTS));
  AudioClip * last_clip =
    AUDIO_POOL->clips[ZRYTHM->undo_stack_len + 4];
  char * last_clip_path =
    audio_clip_get_path_in_pool (
      last_clip, F_NOT_BACKUP);
  g_assert_true (g_file_test (
    last_clip_path, G_FILE_TEST_EXISTS));

  char * undo_manager_yaml = yaml_serialize (
    UNDO_MANAGER, &undo_manager_schema);
  g_message ("\n%s", undo_manager_yaml);

  /* undo and check that last file still exists */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (g_file_test (
    last_clip_path, G_FILE_TEST_EXISTS));
  g_assert_cmpint (
    AUDIO_POOL->num_clips, ==,
    ZRYTHM->undo_stack_len + 5);
  g_assert_nonnull (
    AUDIO_POOL->clips[ZRYTHM->undo_stack_len + 4]);

  /* do another action so that the undoable action
   * for last file no longer exists */
  track_create_with_action (
    TRACK_TYPE_MIDI, NULL, NULL, PLAYHEAD,
    num_tracks_before, 1, NULL);

  /* save the project to remove unused files from
   * pool */
  test_project_save_and_reload ();

  /* assert first and previous last clip are
   * removed */
  g_assert_cmpint (
    AUDIO_POOL->num_clips, ==,
    ZRYTHM->undo_stack_len + 5);
  g_assert_null (AUDIO_POOL->clips[0]);
  g_assert_false (
    g_file_test (clip_path, G_FILE_TEST_EXISTS));
  g_assert_nonnull (AUDIO_POOL->clips[1]);
  g_assert_null (
    AUDIO_POOL->clips[ZRYTHM->undo_stack_len + 4]);
  g_assert_nonnull (
    AUDIO_POOL->clips[ZRYTHM->undo_stack_len + 5]);
  g_assert_true (
    g_file_test (clip_path2, G_FILE_TEST_EXISTS));
  g_assert_false (g_file_test (
    last_clip_path, G_FILE_TEST_EXISTS));

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/pool/"

  g_test_add_func (
    TEST_PREFIX "test remove unused",
    (GTestFunc) test_remove_unused);

  return g_test_run ();
}
