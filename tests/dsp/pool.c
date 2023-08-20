// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/tempo_track.h"
#include "dsp/track.h"
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
  /* on first iteration, run the whole logic
   * on second iteration, just check that no files
   * in the main project are removed when saving
   * a backup */
  for (int i = 0; i < 2; i++)
    {
      test_helper_zrythm_init ();

      /* create many audio tracks with region to push
       * the first few off the undo stack */
      char * filepath = g_build_filename (
        TESTS_SRCDIR, "test_start_with_signal.mp3", NULL);
      SupportedFile * file =
        supported_file_new_from_path (filepath);
      int num_tracks_before = TRACKLIST->num_tracks;
      track_create_with_action (
        TRACK_TYPE_AUDIO, NULL, file, PLAYHEAD,
        num_tracks_before, 1, -1, NULL, NULL);

      /* save and reload the project */
      test_project_save_and_reload ();

      /* delete the first track so that the clip for it
       * will no longer exist (pushed off undo stack) */
      track_select (
        TRACKLIST->tracks[num_tracks_before], F_SELECT,
        F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
      tracklist_selections_action_perform_delete (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

      /* if second iteration, clear undo stacks
       * (happens e.g. when removing an
       * uninstantiated plugin), then attempt to
       * save a backup and then attempt to load the
       * main project again */
      if (i == 1)
        {
          char *   dir = g_strdup (PROJECT->dir);
          GError * err = NULL;
          bool     success = project_save (
            PROJECT, PROJECT->dir, F_NOT_BACKUP, 0,
            F_NO_ASYNC, &err);
          g_assert_true (success);

          undo_manager_clear_stacks (UNDO_MANAGER, F_FREE);

          success = project_save (
            PROJECT, PROJECT->dir, F_BACKUP, 0, F_NO_ASYNC,
            &err);
          g_assert_true (success);
          g_assert_nonnull (PROJECT->backup_dir);

          /* free the project */
          object_free_w_func_and_null (project_free, PROJECT);

          /* load the original project */
          char * prj_filepath =
            g_build_filename (dir, PROJECT_FILE, NULL);
          g_free (dir);
          success = project_load (prj_filepath, false, &err);
          g_assert_true (success);
          g_free (prj_filepath);

          test_helper_zrythm_cleanup ();

          break;
        }

      /* create more tracks */
      for (int j = 0; j < (ZRYTHM->undo_stack_len + 4); j++)
        {
          track_create_with_action (
            TRACK_TYPE_AUDIO, NULL, file, PLAYHEAD,
            TRACKLIST->num_tracks, 1, -1, NULL, NULL);
        }

      /* assert all clips still exist */
      g_assert_cmpint (
        AUDIO_POOL->num_clips, ==, ZRYTHM->undo_stack_len + 5);
      AudioClip * clip = AUDIO_POOL->clips[0];
      char *      clip_path =
        audio_clip_get_path_in_pool (clip, F_NOT_BACKUP);
      g_assert_true (
        g_file_test (clip_path, G_FILE_TEST_EXISTS));
      AudioClip * clip2 = AUDIO_POOL->clips[1];
      char *      clip_path2 =
        audio_clip_get_path_in_pool (clip2, F_NOT_BACKUP);
      g_assert_true (
        g_file_test (clip_path2, G_FILE_TEST_EXISTS));
      AudioClip * last_clip =
        AUDIO_POOL->clips[ZRYTHM->undo_stack_len + 4];
      char * last_clip_path =
        audio_clip_get_path_in_pool (last_clip, F_NOT_BACKUP);
      g_assert_true (
        g_file_test (last_clip_path, G_FILE_TEST_EXISTS));

      char * undo_manager_yaml =
        yaml_serialize (UNDO_MANAGER, &undo_manager_schema);
      g_message ("\n%s", undo_manager_yaml);

      /* undo and check that last file still exists */
      undo_manager_undo (UNDO_MANAGER, NULL);
      g_assert_true (
        g_file_test (last_clip_path, G_FILE_TEST_EXISTS));
      g_assert_cmpint (
        AUDIO_POOL->num_clips, ==, ZRYTHM->undo_stack_len + 5);
      g_assert_nonnull (
        AUDIO_POOL->clips[ZRYTHM->undo_stack_len + 4]);

      /* do another action so that the undoable action
       * for last file no longer exists */
      track_create_with_action (
        TRACK_TYPE_MIDI, NULL, NULL, PLAYHEAD,
        num_tracks_before, 1, -1, NULL, NULL);

      /* save the project to remove unused files from
       * pool */
      test_project_save_and_reload ();

      /* assert first and previous last clip are
       * removed */
      g_assert_cmpint (
        AUDIO_POOL->num_clips, ==, ZRYTHM->undo_stack_len + 5);
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
      g_assert_false (
        g_file_test (last_clip_path, G_FILE_TEST_EXISTS));

      test_helper_zrythm_cleanup ();
    }
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
