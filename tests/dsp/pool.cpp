// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/tracklist_selections.h"
#include "dsp/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/pool");

TEST_CASE ("remove unused")
{
  /* on first iteration, run the whole logic on second iteration, just check
   * that no files in the main project are removed when saving a backup */
  for (int i = 0; i < 2; i++)
    {
      ZrythmFixture ();

      /* create many audio tracks with region to push the first few off the undo
       * stack */
      FileDescriptor file (
        fs::path (TESTS_SRCDIR) / "test_start_with_signal.mp3");
      int num_tracks_before = TRACKLIST->get_num_tracks ();
      Track::create_with_action (
        Track::Type::Audio, nullptr, &file, &PLAYHEAD, num_tracks_before, 1, -1,
        nullptr);

      /* save and reload the project */
      test_project_save_and_reload ();

      /* delete the first track so that the clip for it will no longer exist
       * (pushed off undo stack) */
      TRACKLIST->tracks_[num_tracks_before]->select (true, false, false);

      UNDO_MANAGER->perform (std::make_unique<DeleteTracksAction> (
        *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
        *PORT_CONNECTIONS_MGR));

      /* if second iteration, clear undo stacks (happens e.g. when removing an
       * uninstantiated plugin), then attempt to save a backup and then attempt
       * to load the main project again */
      if (i == 1)
        {
          auto dir = PROJECT->dir_;
          REQUIRE_NOTHROW (
            PROJECT->save (PROJECT->dir_, F_NOT_BACKUP, 0, F_NO_ASYNC));

          UNDO_MANAGER->clear_stacks ();

          REQUIRE_NOTHROW (
            PROJECT->save (PROJECT->dir_, F_BACKUP, 0, F_NO_ASYNC));
          REQUIRE_NONEMPTY (PROJECT->backup_dir_);

          /* free the project */
          PROJECT.reset ();

          /* load the original project */
          auto prj_filepath = fs::path (dir) / PROJECT_FILE;
          test_project_reload (prj_filepath);

          break;
        }

      /* create more tracks */
      for (int j = 0; j < (gZrythm->undo_stack_len_ + 4); j++)
        {
          Track::create_with_action (
            Track::Type::Audio, nullptr, &file, &PLAYHEAD,
            TRACKLIST->get_num_tracks (), 1, -1, nullptr);
        }

      /* assert all clips still exist */
      REQUIRE_SIZE_EQ (AUDIO_POOL->clips_, gZrythm->undo_stack_len_ + 5);
      const auto &clip = AUDIO_POOL->clips_[0];
      const auto  clip_path = clip->get_path_in_pool (false);
      REQUIRE (fs::exists (clip_path));
      const auto &clip2 = AUDIO_POOL->clips_[1];
      const auto  clip_path2 = clip2->get_path_in_pool (false);
      REQUIRE (fs::exists (clip_path2));
      const auto &last_clip = AUDIO_POOL->clips_[gZrythm->undo_stack_len_ + 4];
      const auto  last_clip_path = last_clip->get_path_in_pool (false);
      REQUIRE (fs::exists (last_clip_path));

      /* undo and check that last file still exists */
      UNDO_MANAGER->undo ();
      REQUIRE (fs::exists (last_clip_path));
      REQUIRE_SIZE_EQ (AUDIO_POOL->clips_, gZrythm->undo_stack_len_ + 5);
      REQUIRE_NONNULL (AUDIO_POOL->clips_[gZrythm->undo_stack_len_ + 4]);

      /* do another action so that the undoable action for last file no longer
       * exists */
      Track::create_with_action (
        Track::Type::Midi, nullptr, nullptr, &PLAYHEAD, num_tracks_before, 1,
        -1, nullptr);

      /* save the project to remove unused files from pool */
      test_project_save_and_reload ();

      /* assert first and previous last clip are removed */
      REQUIRE_SIZE_EQ (AUDIO_POOL->clips_, gZrythm->undo_stack_len_ + 5);
      REQUIRE_NULL (AUDIO_POOL->clips_[0]);
      REQUIRE_FALSE (fs::exists (clip_path));
      REQUIRE_NONNULL (AUDIO_POOL->clips_[1]);
      REQUIRE_NULL (AUDIO_POOL->clips_[gZrythm->undo_stack_len_ + 4]);
      REQUIRE (fs::exists (clip_path2));
      REQUIRE_FALSE (fs::exists (last_clip_path));
    }
}

TEST_SUITE_END;