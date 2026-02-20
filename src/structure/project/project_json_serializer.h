// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

#include "utils/version.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
{
class Project;

using namespace std::string_view_literals;

/**
 * @brief Handles serialization, deserialization, and validation of Project JSON.
 *
 * This class manages converting Project objects to/from JSON format according
 * to the project schema (data/schemas/project.schema.json), and validates
 * JSON against that schema.
 */
class ProjectJsonSerializer
{
public:
  static constexpr utils::Version SCHEMA_VERSION{ 2, 1, {} };
  static constexpr auto           DOCUMENT_TYPE = "ZrythmProject"sv;
  static constexpr auto           kProjectData = "projectData"sv;
  static constexpr auto           kDatetimeKey = "datetime"sv;
  static constexpr auto           kTitle = "title"sv;

  /**
   * @brief Returns a json representation of the project.
   *
   * @param project The project to serialize.
   * @param app_version Version of the application.
   * @param title Project title.
   * @throw std::runtime_error on error.
   */
  static nlohmann::json serialize (
    const Project        &project,
    const utils::Version &app_version,
    std::string_view      title);

  /**
   * @brief Validates JSON against the project schema.
   *
   * @param j The JSON to validate.
   * @throw std::runtime_error if validation fails.
   */
  static void validate_json (const nlohmann::json &j);

  /**
   * @brief Loads and validates a project from JSON.
   *
   * @param j The JSON to deserialize.
   * @param project The project instance to populate.
   * @throw std::runtime_error on validation or load error.
   */
  static void deserialize (const nlohmann::json &j, Project &project);
};
}
