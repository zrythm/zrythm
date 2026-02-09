// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/progress_info.h"

#include <QFuture>

namespace zrythm::structure::project
{
class Project;

class ProjectSaver
{
public:
  ProjectSaver (const Project &project, std::string_view app_version);

  /**
   * Saves the project asynchronously to the directory set previously in Project.
   *
   * @param path The directory to save the project in (including the title).
   * @param is_backup 1 if this is a backup. Backups will be saved as <original
   * filename>.bak<num>.
   *
   * @throw ZrythmException If any step failed.
   */
  [[nodiscard]] QFuture<utils::Utf8String>
  save (const fs::path &path, bool is_backup);

  /**
   * Autosave callback.
   *
   * This will keep getting called at regular short intervals, and if enough
   * time has passed and it's okay to save it will autosave, otherwise it will
   * wait until the next interval and check again.
   */
  static int autosave_cb (void * data);

  /**
   * @brief Creates the project directories.
   *
   * @throw ZrythmException If the directories cannot be created.
   */
  static void make_project_dirs (const fs::path &project_directory);

  /**
   * Compresses/decompress a project from a file/data to a file/data.
   *
   * @param compress True to compress, false to decompress.
   * @param[out] _dest Pointer to a location to allocate memory.
   * @param[out] _dest_size Pointer to a location to store the size of the
   * allocated memory.
   * @param src Input bytes to compress/decompress.
   *
   * @throw ZrythmException If the compression/decompression fails.
   */
  static void compress_or_decompress (
    bool              compress,
    char **           _dest,
    size_t *          _dest_size,
    const QByteArray &src);

  static void
  compress (char ** _dest, size_t * _dest_size, const QByteArray &src)
  {
    compress_or_decompress (true, _dest, _dest_size, src);
  }

  static void
  decompress (char ** _dest, size_t * _dest_size, const QByteArray &src)
  {
    compress_or_decompress (false, _dest, _dest_size, src);
  }

  /**
   * Returns the uncompressed text representation of the saved project file.
   *
   * @param backup Whether to use the project file from the most recent
   * backup.
   *
   * @throw ZrythmException If an error occurs.
   */
  static std::string
  get_existing_uncompressed_text (const fs::path &project_dir);

  bool has_unsaved_changes () const;

private:
  /**
   * Sets (and creates on the disk) the next available backup dir to use for
   * saving a backup during this call.
   *
   * @throw ZrythmException If the backup dir cannot be created.
   */
  void set_and_create_next_available_backup_dir ();

  /**
   * Cleans up unnecessary plugin state dirs from the main project.
   *
   * This is called during save on the newly cloned project to be saved.
   *
   * @param main_project The main project.
   * @param is_backup Whether this is for a backup.
   */
  void cleanup_plugin_state_dirs (
    const Project  &main_project,
    const fs::path &project_dir,
    bool            is_backup);

private:
  const Project &project_;
  // std::optional<gui::actions::UndoableActionPtrVariant>
  // last_action_in_last_successful_autosave_;

  /** Last successful autosave timestamp. */
  SteadyTimePoint last_successful_autosave_time_;

  /** Used to check if the project has unsaved changes. */
  // std::optional<gui::actions::UndoableActionPtrVariant> last_saved_action_;

  /** Semaphore used to block saving. */
  std::binary_semaphore save_sem_{ 1 };

  std::string app_version_;
};
}
