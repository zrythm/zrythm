// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <expected>
#include <filesystem>
#include <memory>

#include "utils/isettings_backend.h"
#include "utils/utf8_string.h"

namespace zrythm::utils
{

/**
 * @brief The directories that hold the settings, data and caches of a run.
 *
 * The settings store, the data directory and the cache directory come from
 * this class.
 */
class UserDirectories
{
public:
  /**
   * @brief Uses the locations of the operating system for this application.
   *
   * The application must have an application name or an organization name
   * when a path is requested, because those locations contain them.
   */
  UserDirectories () = default;

  /**
   * @brief Keeps everything under one directory.
   *
   * @param directory An absolute path, or a path relative to the current
   * directory. It must not be empty.
   *
   * A missing directory and missing "data" and "cache" subdirectories are
   * created. The disk caches of Qt Quick are written to the "cache"
   * subdirectory.
   *
   * @return The directories, or an error when @p directory is empty or a
   * directory cannot be created.
   */
  static std::expected<UserDirectories, Utf8String>
  make_private (const std::filesystem::path &directory);

  /** Returns whether everything is kept under one directory. */
  [[nodiscard]] bool is_private () const;

  /**
   * Returns an opened settings store: "settings.ini" for a private
   * directory, the settings store of the operating system otherwise.
   *
   * @throw ZrythmException When the settings store of the operating system
   * is used and the application has no application name and no
   * organization name.
   */
  [[nodiscard]] std::unique_ptr<ISettingsBackend>
  make_settings_backend () const;

  /**
   * Returns the directory of data that must be kept, such as the audio
   * device setup.
   *
   * @throw ZrythmException When the application has no application name
   * and no organization name, or when the operating system has no such
   * location.
   */
  [[nodiscard]] std::filesystem::path data_dir () const;

  /**
   * Returns the directory of files that can be rebuilt, such as the log.
   *
   * @throw ZrythmException When the application has no application name
   * and no organization name, or when the operating system has no such
   * location.
   */
  [[nodiscard]] std::filesystem::path cache_dir () const;

  /**
   * Returns the directory of the cache of scanned plugins.
   *
   * @throw ZrythmException When the application has no application name
   * and no organization name, or when the operating system has no such
   * location.
   */
  [[nodiscard]] std::filesystem::path plugin_scan_cache_dir () const;

  /**
   * Returns the directory where new projects are created by default.
   *
   * The directory is the documents directory of the user with the name of
   * the application. A private directory does not change this location.
   *
   * @throw ZrythmException When the application has no application name,
   * or when the operating system has no documents location.
   */
  [[nodiscard]] std::filesystem::path default_new_project_dir () const;

  /**
   * Requests every location once.
   *
   * @throw ZrythmException When a location cannot be given, for a reason
   * given with that location.
   */
  void check_locations () const;

private:
  /**
   * Root of the private layout.
   *
   * Empty for the locations of the operating system.
   */
  std::filesystem::path private_dir_{};
};

} // namespace zrythm::utils
