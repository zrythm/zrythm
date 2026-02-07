// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project_json_serializer.h"
#include "utils/gtest_wrapper.h"

#include <nlohmann/json.hpp>

using namespace zrythm::structure::project;
using namespace std::string_view_literals;

TEST (ProjectJsonSerializerTest, Constants)
{
  // Test static constants
  EXPECT_EQ (ProjectJsonSerializer::FORMAT_MAJOR_VERSION, 2);
  EXPECT_EQ (ProjectJsonSerializer::FORMAT_MINOR_VERSION, 1);
  EXPECT_EQ (ProjectJsonSerializer::DOCUMENT_TYPE, "ZrythmProject"sv);
  EXPECT_EQ (ProjectJsonSerializer::kProjectData, "projectData"sv);
  EXPECT_EQ (ProjectJsonSerializer::kDatetimeKey, "datetime"sv);
  EXPECT_EQ (ProjectJsonSerializer::kAppVersionKey, "appVersion"sv);
}

TEST (ProjectJsonSerializerTest, ConstantValidation)
{
  // Verify that all required fields are present in the serializer constants

  // Document type should be non-empty and match expected value
  EXPECT_FALSE (ProjectJsonSerializer::DOCUMENT_TYPE.empty ());
  EXPECT_EQ (ProjectJsonSerializer::DOCUMENT_TYPE, "ZrythmProject"sv);

  // Format versions should be reasonable
  EXPECT_GE (ProjectJsonSerializer::FORMAT_MAJOR_VERSION, 1);
  EXPECT_GE (ProjectJsonSerializer::FORMAT_MINOR_VERSION, 0);
  EXPECT_LE (ProjectJsonSerializer::FORMAT_MINOR_VERSION, 99);

  // All keys should be non-empty
  EXPECT_FALSE (ProjectJsonSerializer::kProjectData.empty ());
  EXPECT_FALSE (ProjectJsonSerializer::kDatetimeKey.empty ());
  EXPECT_FALSE (ProjectJsonSerializer::kAppVersionKey.empty ());
}

TEST (ProjectJsonSerializerTest, ValidateValidJson)
{
  // Create a minimal valid project JSON
  nlohmann::json j;
  j["documentType"] = "ZrythmProject";
  j["formatMajor"] = 2;
  j["formatMinor"] = 1;
  j["appVersion"] = "2.0.0";
  j["datetime"] = "2026-01-28T12:00:00Z";
  j["title"] = "Test Project";
  j[ProjectJsonSerializer::kProjectData] = nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["tempoMap"] = nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["tempoMap"]["timeSignatures"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["tempoMap"]["tempoChanges"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["transport"] =
    nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["tracklist"] =
    nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["tracklist"]["tracks"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["tracklist"]["pinnedTracksCutoff"] = 0;
  j[ProjectJsonSerializer::kProjectData]["registries"] =
    nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["registries"]["portRegistry"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]["paramRegistry"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]["pluginRegistry"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]["trackRegistry"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]
   ["arrangerObjectRegistry"] = nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]
   ["fileAudioSourceRegistry"] = nlohmann::json::array ();

  // Should not throw
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerTest, ValidateInvalidJson)
{
  // Create an invalid project JSON (missing required fields)
  nlohmann::json j;
  j["documentType"] = "ZrythmProject";
  // Missing required fields like formatMajor, formatMinor, etc.

  // Should throw
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}
