// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

namespace zrythm::structure::project
{
class ProjectPathProvider
{
  static constexpr auto PROJECT_FILE = "project.zpj"sv;
  static constexpr auto PROJECT_BACKUPS_DIR = "backups"sv;
  static constexpr auto PROJECT_PLUGINS_DIR = "plugins"sv;
  static constexpr auto PROJECT_PLUGIN_STATES_DIR = "states"sv;
  static constexpr auto PROJECT_PLUGIN_EXT_COPIES_DIR = "ext_file_copies"sv;
  static constexpr auto PROJECT_PLUGIN_EXT_LINKS_DIR = "ext_file_links"sv;
  static constexpr auto PROJECT_EXPORTS_DIR = "exports"sv;
  static constexpr auto PROJECT_STEMS_DIR = "stems"sv;
  static constexpr auto PROJECT_POOL_DIR = "pool"sv;

public:
  enum class ProjectPath
  {
    /**
     * @brief The project .zpj file.
     */
    ProjectFile,

    BackupsDir,

    /** Plugins path (directory). */
    PluginsDir,

    /** Path for state .ttl files. */
    PluginStates,

    /** External files for plugin states, under the
     * STATES dir. */
    PLUGIN_EXT_COPIES,

    /** External files for plugin states, under the
     * STATES dir. */
    PLUGIN_EXT_LINKS,

    ExportsDir,

    /* EXPORTS / "stems". */
    ExportStemsDir,

    AudioFilePoolDir,
  };

  /**
   * @brief Returns the path of the given type relative to a project's directory.
   */
  static fs::path get_path (ProjectPath path);
};
}
