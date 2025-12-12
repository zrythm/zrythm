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
  ProjectSaver (const Project &project);

  QFuture<void> save_async (bool for_backup);

  /**
   * Saves the project to the directory set previously in Project.
   *
   * @param is_backup 1 if this is a backup. Backups will be saved as <original
   * filename>.bak<num>.
   * @param show_notification Show a notification in the UI that the project
   * was saved.
   * @param async Save asynchronously in another thread.
   *
   * @throw ZrythmException If the project cannot be saved.
   */
  void save (bool is_backup, bool show_notification, bool async);

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
   * @param is_backup
   * @throw ZrythmException If the directories cannot be created.
   */
  static void make_project_dirs (const Project &project, bool is_backup);

  /**
   * Flag to pass to project_compress() and project_decompress().
   */
  enum class CompressionFlag
  {
    PROJECT_COMPRESS_FILE = 0,
    PROJECT_DECOMPRESS_FILE = 0,
    PROJECT_COMPRESS_DATA = 1,
    PROJECT_DECOMPRESS_DATA = 1,
  };

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
    CompressionFlag   dest_type,
    const QByteArray &src);

  static void compress (
    char **           _dest,
    size_t *          _dest_size,
    CompressionFlag   dest_type,
    const QByteArray &src)
  {
    compress_or_decompress (true, _dest, _dest_size, dest_type, src);
  }

  static void decompress (
    char **           _dest,
    size_t *          _dest_size,
    CompressionFlag   dest_type,
    const QByteArray &src)
  {
    compress_or_decompress (false, _dest, _dest_size, dest_type, src);
  }

  /**
   * Returns the uncompressed text representation of the saved project file.
   *
   * @param backup Whether to use the project file from the most recent
   * backup.
   *
   * @throw ZrythmException If an error occurs.
   */
  std::string get_existing_uncompressed_text (bool backup);

  bool has_unsaved_changes () const;

private:
  /**
   * Project save data.
   */
  struct SaveContext
  {
    /** Project clone (with memcpy). */
    // std::unique_ptr<Project> project_;

    /**
     * @brief Original project.
     */
    const Project * main_project_;

    /** Full path to save to. */
    fs::path project_file_path_;

    bool is_backup_ = false;

    /** To be set to true when the thread finishes. */
    std::atomic_bool finished_ = false;

    bool show_notification_ = false;

    /** Whether an error occurred during saving. */
    bool has_error_ = false;

    ProgressInfo progress_info_;
  };

  /**
   * Thread that does the serialization and saving.
   */
  class SerializeProjectThread final : public juce::Thread
  {
  public:
    SerializeProjectThread (SaveContext &ctx);
    ~SerializeProjectThread () override;
    void run () override;

  private:
    SaveContext &ctx_;
  };

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
  void cleanup_plugin_state_dirs (const Project &main_project, bool is_backup);

  /**
   * Idle func to check if the project has finished saving and show a
   * notification.
   *
   * @return Whether to continue calling this.
   */
  static bool idle_saved_callback (SaveContext * ctx);

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
};
}
