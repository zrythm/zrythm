// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/track.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

class AudioPoolRemoveUnusedTest
    : public ZrythmFixture,
      public ::testing::WithParamInterface<int>
{
protected:
  void SetUp () override
  {
    ZrythmFixture::SetUp ();
    iteration = GetParam ();
  }

  int iteration;
};

/* on first iteration, run the whole logic on second iteration, just check
 * that no files in the main project are removed when saving a backup */
TEST_P (AudioPoolRemoveUnusedTest, RemoveUnused)
{
  /* create many audio tracks with region to push the first few off the
   * undo stack */
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test_start_with_signal.mp3");
  int            num_tracks_before = TRACKLIST->get_num_tracks ();
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &PLAYHEAD, num_tracks_before, 1, -1,
    nullptr);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* delete the first track so that the clip for it will no longer exist
   * (pushed off undo stack) */
  TRACKLIST->tracks_[num_tracks_before]->select (true, false, false);

  UNDO_MANAGER->perform (
    std::make_unique<DeleteTracksAction> (
      *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR));

  /* if second iteration, clear undo stacks (happens e.g. when removing an
   * uninstantiated plugin), then attempt to save a backup and then
   * attempt to load the main project again */
  if (iteration == 1)
    {
      auto dir = PROJECT->dir_;
      ASSERT_NO_THROW (
        PROJECT->save (PROJECT->dir_, F_NOT_BACKUP, 0, F_NO_ASYNC));

      UNDO_MANAGER->clear_stacks ();

      ASSERT_NO_THROW (PROJECT->save (PROJECT->dir_, F_BACKUP, 0, F_NO_ASYNC));
      ASSERT_NONEMPTY (PROJECT->backup_dir_);

      /* free the project */
      AUDIO_ENGINE->activate (false);
      PROJECT.reset ();

      /* load the original project */
      auto prj_filepath = fs::path (dir) / PROJECT_FILE;
      test_project_reload (prj_filepath);

      return;
    }

  /* create more tracks */
  for (int j = 0; j < (gZrythm->undo_stack_len_ + 4); j++)
    {
      Track::create_with_action (
        Track::Type::Audio, nullptr, &file, &PLAYHEAD,
        TRACKLIST->get_num_tracks (), 1, -1, nullptr);
    }

  /* assert all clips still exist */
  ASSERT_SIZE_EQ (AUDIO_POOL->clips_, gZrythm->undo_stack_len_ + 5);
  const auto &clip = AUDIO_POOL->clips_[0];
  const auto  clip_path = clip->get_path_in_pool (false);
  ASSERT_TRUE (fs::exists (clip_path));
  const auto &clip2 = AUDIO_POOL->clips_[1];
  const auto  clip_path2 = clip2->get_path_in_pool (false);
  ASSERT_TRUE (fs::exists (clip_path2));
  const auto &last_clip = AUDIO_POOL->clips_[gZrythm->undo_stack_len_ + 4];
  const auto  last_clip_path = last_clip->get_path_in_pool (false);
  ASSERT_TRUE (fs::exists (last_clip_path));

  /* undo and check that last file still exists */
  UNDO_MANAGER->undo ();
  ASSERT_TRUE (fs::exists (last_clip_path));
  ASSERT_SIZE_EQ (AUDIO_POOL->clips_, gZrythm->undo_stack_len_ + 5);
  ASSERT_NONNULL (AUDIO_POOL->clips_[gZrythm->undo_stack_len_ + 4]);

  /* do another action so that the undoable action for last file no longer
   * exists */
  Track::create_with_action (
    Track::Type::Midi, nullptr, nullptr, &PLAYHEAD, num_tracks_before, 1, -1,
    nullptr);

  /* save the project to remove unused files from pool */
  test_project_save_and_reload ();

  /* assert first and previous last clip are removed */
  ASSERT_SIZE_EQ (AUDIO_POOL->clips_, gZrythm->undo_stack_len_ + 5);
  ASSERT_NULL (AUDIO_POOL->clips_[0]);
  ASSERT_FALSE (fs::exists (clip_path));
  ASSERT_NONNULL (AUDIO_POOL->clips_[1]);
  ASSERT_NULL (AUDIO_POOL->clips_[gZrythm->undo_stack_len_ + 4]);
  ASSERT_TRUE (fs::exists (clip_path2));
  ASSERT_FALSE (fs::exists (last_clip_path));
}

INSTANTIATE_TEST_SUITE_P (
  AudioPoolRemoveUnused,
  AudioPoolRemoveUnusedTest,
  ::testing::Values (0, 1));
