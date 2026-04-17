// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <string_view>

namespace zrythm::structure::project
{
using namespace std::string_view_literals;

class ProjectPathProvider
{
  static constexpr auto PROJECT_FILE = "project.zpj"sv;
  static constexpr auto PROJECT_BACKUPS_DIR = "backups"sv;
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

    ExportsDir,

    /* EXPORTS / "stems". */
    ExportStemsDir,

    AudioFilePoolDir,
  };

  /**
   * @brief Returns the path of the given type relative to a project's directory.
   */
  static std::filesystem::path get_path (ProjectPath path);
};
}
