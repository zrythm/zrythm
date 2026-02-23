// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <string>

#include "utils/types.h"
#include "utils/utf8_string.h"
#include "utils/version.h"

#include <QByteArray>
#include <QFuture>

namespace zrythm::structure::project
{
class Project;
class ProjectUiState;
}

namespace zrythm::undo
{
class UndoStack;
}

namespace zrythm::controllers
{

/**
 * @brief Handles saving of Zrythm projects to disk.
 *
 * This class orchestrates the complete project saving pipeline:
 * 1. Creating project directories
 * 2. Writing audio pool files
 * 3. Writing plugin states
 * 4. Serializing project JSON (Project + ProjectUiState + UndoStack)
 * 5. Compressing and writing to disk
 */
class ProjectSaver
{
public:
  /**
   * @brief Constructs a ProjectSaver for the given project components.
   *
   * @param project The core project data to save.
   * @param ui_state The UI state to save.
   * @param undo_stack The undo history to save.
   * @param app_version Version of the application.
   */
  ProjectSaver (
    const structure::project::Project        &project,
    const structure::project::ProjectUiState &ui_state,
    const undo::UndoStack                    &undo_stack,
    utils::Version                            app_version);

  /**
   * Saves the project asynchronously to the specified directory.
   *
   * @param path The directory to save the project in (including the title).
   * @param is_backup True if this is a backup. Backups will be saved as
   *                  <original filename>.bak<num>.
   *
   * @return A QFuture that resolves to the project title on success.
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
   * @param project_directory The root project directory.
   * @throw ZrythmException If the directories cannot be created.
   */
  static void make_project_dirs (const fs::path &project_directory);

  /**
   * Compresses or decompresses project data using zstd.
   *
   * @param compress True to compress, false to decompress.
   * @param[out] _dest Pointer to a location to allocate memory.
   * @param[out] _dest_size Pointer to a location to store the size of the
   *                        allocated memory.
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
   * @param project_dir The project directory.
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
   * @param main_project The main project.
   * @param project_dir The project directory being saved to.
   * @param is_backup Whether this is for a backup.
   */
  void cleanup_plugin_state_dirs (
    const structure::project::Project &main_project,
    const fs::path                    &project_dir,
    bool                               is_backup);

private:
  const structure::project::Project        &project_;
  const structure::project::ProjectUiState &ui_state_;
  const undo::UndoStack                    &undo_stack_;

  /** Last successful autosave timestamp. */
  SteadyTimePoint last_successful_autosave_time_;

  /** Semaphore used to block saving. */
  std::binary_semaphore save_sem_{ 1 };

  utils::Version app_version_;
};

}
