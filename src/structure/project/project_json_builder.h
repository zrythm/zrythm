// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
{
class Project;

/**
 * @brief Helper for building a JSON representation of a Project.
 */
class ProjectJsonBuilder
{
public:
  static constexpr auto FORMAT_MAJOR_VERSION = 2;
  static constexpr auto FORMAT_MINOR_VERSION = 1;
  static constexpr auto DOCUMENT_TYPE = "ZrythmProject"sv;
  static constexpr auto kProject = "project"sv;
  static constexpr auto kDatetimeKey = "datetime"sv;
  static constexpr auto kAppVersionKey = "appVersion"sv;

  /**
   * @brief Returns a json representation of the project.
   *
   * @param project The project to serialize.
   * @throw std::runtime_error on error.
   */
  static nlohmann::json
  build_json (const Project &project, std::string_view app_version);
};
}
